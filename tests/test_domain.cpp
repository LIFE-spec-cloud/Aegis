#include "catch_compat.hpp"
#include "domain/Commodity.h"
#include "domain/CropFreshnessState.h"
#include "domain/CropBatch.h"
#include "domain/RoadGraph.h"
#include "domain/PriceModel.h"
#include "strategy/IRoutePlanningStrategy.h"
#include "strategy/IPricingStrategy.h"

#include <cmath>
#include <stdexcept>

using namespace kisansetu::contracts;
using namespace kisansetu::domain;
using namespace kisansetu::strategy;

TEST_CASE("CropFreshnessState_ExactGradeBoundaries", "[domain]") {
    // Grade A: Q >= 0.85 (Price multiplier: 1.00)
    auto stateA_exact = CropFreshnessState::fromQuality(0.85);
    CHECK(stateA_exact->grade() == FreshnessGrade::GradeA);
    CHECK(stateA_exact->priceMultiplier() == 1.00);

    auto stateA_high = CropFreshnessState::fromQuality(0.99);
    CHECK(stateA_high->grade() == FreshnessGrade::GradeA);

    // Just below 0.85 -> Grade B
    auto stateB_high = CropFreshnessState::fromQuality(0.8499);
    CHECK(stateB_high->grade() == FreshnessGrade::GradeB);
    CHECK(stateB_high->priceMultiplier() == 0.75);

    // Grade B: Q >= 0.65 (Price multiplier: 0.75)
    auto stateB_exact = CropFreshnessState::fromQuality(0.65);
    CHECK(stateB_exact->grade() == FreshnessGrade::GradeB);
    CHECK(stateB_exact->priceMultiplier() == 0.75);

    // Just below 0.65 -> Grade C
    auto stateC_high = CropFreshnessState::fromQuality(0.6499);
    CHECK(stateC_high->grade() == FreshnessGrade::GradeC);
    CHECK(stateC_high->priceMultiplier() == 0.45);

    // Grade C: Q >= 0.40 (Price multiplier: 0.45)
    auto stateC_exact = CropFreshnessState::fromQuality(0.40);
    CHECK(stateC_exact->grade() == FreshnessGrade::GradeC);
    CHECK(stateC_exact->priceMultiplier() == 0.45);

    // Spoiled: Q < 0.40 (Price multiplier: 0.00)
    auto stateSpoiled_high = CropFreshnessState::fromQuality(0.3999);
    CHECK(stateSpoiled_high->grade() == FreshnessGrade::Spoiled);
    CHECK(stateSpoiled_high->priceMultiplier() == 0.00);

    auto stateSpoiled_zero = CropFreshnessState::fromQuality(0.00);
    CHECK(stateSpoiled_zero->grade() == FreshnessGrade::Spoiled);
}

TEST_CASE("CropFreshnessState_IllegalTransitionsRejected", "[domain]") {
    // 1. Grade A can decay to any lower grade
    auto stateA = CropFreshnessState::create(FreshnessGrade::GradeA);
    CHECK(stateA->canTransitionTo(FreshnessGrade::GradeA));
    CHECK(stateA->canTransitionTo(FreshnessGrade::GradeB));
    CHECK(stateA->canTransitionTo(FreshnessGrade::GradeC));
    CHECK(stateA->canTransitionTo(FreshnessGrade::Spoiled));

    // 2. Grade B can decay to B, C, Spoiled, but CANNOT recover to Grade A
    auto stateB = CropFreshnessState::create(FreshnessGrade::GradeB);
    CHECK(stateB->canTransitionTo(FreshnessGrade::GradeB));
    CHECK(stateB->canTransitionTo(FreshnessGrade::GradeC));
    CHECK(stateB->canTransitionTo(FreshnessGrade::Spoiled));
    CHECK(!stateB->canTransitionTo(FreshnessGrade::GradeA));

    bool caughtBA = false;
    try {
        [[maybe_unused]] auto next = stateB->transitionTo(FreshnessGrade::GradeA);
    } catch (const std::logic_error&) {
        caughtBA = true;
    }
    CHECK(caughtBA);

    // 3. Grade C can transition to C, Spoiled, but CANNOT recover to Grade A or B
    auto stateC = CropFreshnessState::create(FreshnessGrade::GradeC);
    CHECK(stateC->canTransitionTo(FreshnessGrade::GradeC));
    CHECK(stateC->canTransitionTo(FreshnessGrade::Spoiled));
    CHECK(!stateC->canTransitionTo(FreshnessGrade::GradeA));
    CHECK(!stateC->canTransitionTo(FreshnessGrade::GradeB));

    bool caughtCB = false;
    try {
        [[maybe_unused]] auto next = stateC->transitionTo(FreshnessGrade::GradeB);
    } catch (const std::logic_error&) {
        caughtCB = true;
    }
    CHECK(caughtCB);

    // 4. Spoiled is terminal: cannot recover to A, B, or C
    auto stateSpoiled = CropFreshnessState::create(FreshnessGrade::Spoiled);
    CHECK(stateSpoiled->canTransitionTo(FreshnessGrade::Spoiled));
    CHECK(!stateSpoiled->canTransitionTo(FreshnessGrade::GradeA));
    CHECK(!stateSpoiled->canTransitionTo(FreshnessGrade::GradeB));
    CHECK(!stateSpoiled->canTransitionTo(FreshnessGrade::GradeC));

    bool caughtSA = false;
    try {
        [[maybe_unused]] auto next = stateSpoiled->transitionTo(FreshnessGrade::GradeA);
    } catch (const std::logic_error&) {
        caughtSA = true;
    }
    CHECK(caughtSA);
}

TEST_CASE("DecayFunction_CalibratedReferenceValues", "[domain]") {
    Commodity tomato = Commodity::fromType(CommodityType::Tomato);
    CHECK(tomato.lambda == 0.030);
    CHECK(tomato.basePricePerQuintal.toRupees() == 2200.0);

    CropBatch batch("BATCH-TOM-01", tomato, Quintals(50.0), SimTime(0));
    CHECK(batch.currentQuality() == 1.0);

    // Calibrated Fault 1 check:
    // Temp = 28 C -> mu_temp = 1.0 + 0.5 * (8/20) = 1.20
    double mu = CropBatch::calculateMuTemp(28.0, false);
    CHECK(std::abs(mu - 1.20) < 1e-4);

    // 8 hours transit -> Q = exp(-0.030 * 8 * 1.2) = exp(-0.288) = 0.74976 ≈ 0.75 (Grade B)
    double q8h = batch.predictQualityAt(SimTime(8 * 60), 28.0, false);
    CHECK(std::abs(q8h - 0.75) < 0.01);
    CHECK(CropFreshnessState::fromQuality(q8h)->grade() == FreshnessGrade::GradeB);

    // Injected stall adds 7 hours (total 15 hours) -> Q = exp(-0.030 * 15 * 1.2) = exp(-0.540) = 0.5827 ≈ 0.583
    // Below mandi_min_quality (0.60), triggers distress auction
    double q15h = batch.predictQualityAt(SimTime(15 * 60), 28.0, false);
    CHECK(std::abs(q15h - 0.583) < 0.01);
    CHECK(q15h < 0.60); // below mandi acceptance threshold

    // Refrigerated transport: mu_temp is multiplied by 0.6
    double muRef = CropBatch::calculateMuTemp(28.0, true);
    CHECK(std::abs(muRef - (1.20 * 0.6)) < 1e-4);

    double q15h_ref = batch.predictQualityAt(SimTime(15 * 60), 28.0, true);
    // Q = exp(-0.030 * 15 * 0.72) = exp(-0.324) ≈ 0.723 (remains Grade B, well above 0.60!)
    CHECK(q15h_ref > 0.70);
    CHECK(q15h_ref > 0.60);
}

TEST_CASE("RoadGraph_MutableWeights", "[domain]") {
    RoadGraph graph;
    NodeId a("NodeA");
    NodeId b("NodeB");

    graph.addEdge(a, b, 100.0, SimTime(120), Money::fromRupees(150.0));
    auto edgeOpt = graph.getEdge(a, b);
    REQUIRE(edgeOpt.has_value());
    CHECK(edgeOpt->distanceKm == 100.0);
    CHECK(edgeOpt->effectiveTravelTime().ticks() == 120);
    CHECK(edgeOpt->tollCost.toRupees() == 150.0);

    // Apply 2.0x congestion multiplier
    graph.setCongestion(a, b, 2.0);
    auto congestedEdge = graph.getEdge(a, b);
    REQUIRE(congestedEdge.has_value());
    CHECK(congestedEdge->effectiveTravelTime().ticks() == 240);

    // Update toll
    graph.setToll(a, b, Money::fromRupees(200.0));
    CHECK(graph.getEdge(a, b)->tollCost.toRupees() == 200.0);
}

TEST_CASE("StrategySwapping_ChangesRouteOnSameGraph", "[domain]") {
    // Construct a scenario with two parallel corridors from Origin to Dest:
    // Route 1 (Express Highway): 50 km, travelTime = 30 min, toll = ₹500
    // Route 2 (Country Road):    60 km, travelTime = 55 min, toll = ₹0

    RoadGraph graph;
    NodeId origin("Origin");
    NodeId dest("Destination");
    NodeId jctHwy("Jct_Highway");
    NodeId jctCountry("Jct_Country");

    // Express Highway via jctHwy
    graph.addEdge(origin, jctHwy, 25.0, SimTime(15), Money::fromRupees(250.0));
    graph.addEdge(jctHwy, dest, 25.0, SimTime(15), Money::fromRupees(250.0));

    // Country road via jctCountry
    graph.addEdge(origin, jctCountry, 30.0, SimTime(25), Money::fromRupees(0.0));
    graph.addEdge(jctCountry, dest, 30.0, SimTime(30), Money::fromRupees(0.0));

    // Strategy 1: Dijkstra (Minimizes travel time)
    auto dijkstra = IRoutePlanningStrategy::create(RouteStrategyType::DIJKSTRA);
    RouteInfo dijkstraRoute = dijkstra->planRoute(graph, origin, dest);
    REQUIRE(!dijkstraRoute.path.empty());
    // Dijkstra must pick Highway (30 min < 55 min)
    CHECK(dijkstraRoute.estimatedTransitTime.ticks() == 30);
    CHECK(dijkstraRoute.path[1] == jctHwy);
    CHECK(dijkstraRoute.tollCost.toRupees() == 500.0);

    // Strategy 2: TollAverse (Penalizes tolls heavily)
    auto tollAverse = IRoutePlanningStrategy::create(RouteStrategyType::TOLL_AVERSE);
    RouteInfo tollAverseRoute = tollAverse->planRoute(graph, origin, dest);
    REQUIRE(!tollAverseRoute.path.empty());
    // TollAverse must pick Country Road (₹0 toll < ₹500 toll)
    CHECK(tollAverseRoute.path[1] == jctCountry);
    CHECK(tollAverseRoute.tollCost.toRupees() == 0.0);
    CHECK(tollAverseRoute.estimatedTransitTime.ticks() == 55);

    // Verified: Swapping strategy changed the route from Highway to Country Road
    // on the EXACT same graph without modifying any agent or graph code!
    CHECK(dijkstraRoute.path != tollAverseRoute.path);
}

TEST_CASE("PriceModel_SurgeAndNetMargin", "[domain]") {
    Money basePrice = Money::fromRupees(2200.0);

    // 0 to 4 arrivals (<= capacity of 4): no surge discount
    Money pNormal = PriceModel::calculateSurgePrice(basePrice, 3.0, 4.0, 0.50, 0.40);
    CHECK(pNormal.toRupees() == 2200.0);

    // 50 unannounced arrivals (Fault 2 condition)
    // ratio = 50 / 4 = 12.5; overload = 11.5; discount = 0.5 * 11.5 = 5.75
    // multiplier clamped to floor 0.40
    Money pGlut = PriceModel::calculateSurgePrice(basePrice, 50.0, 4.0, 0.50, 0.40);
    CHECK(pGlut.toRupees() == 880.0); // 0.40 * 2200

    // Net margin calculation:
    // P = ₹2000, Grade B (multiplier 0.75) -> effective price ₹1500
    // Volume = 50 quintals -> gross payout = ₹75,000
    // TransitCost = ₹5,000, MandiFees = ₹2,000 -> NetMargin = ₹68,000
    Money net = PriceModel::calculateNetMargin(
        Money::fromRupees(2000.0),
        0.75,
        Quintals(50.0),
        Money::fromRupees(5000.0),
        Money::fromRupees(2000.0)
    );
    CHECK(net.toRupees() == 68000.0);
}
