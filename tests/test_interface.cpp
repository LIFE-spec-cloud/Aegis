#include "../ui_adapter/VernacularLexicon.h"
#include "../ui_adapter/VoiceKioskAdapter.h"
#include "../ui_adapter/ProduceStatusCard.h"
#include "../observer/AudioSpeakerNotifier.h"
#include "../simulation/Dashboard.h"
#include "../include/contracts/EventLog.h"

#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace agro;

void testVernacularVoiceParsing20PlusPhrases() {
    std::cout << "[RUN] Testing Vernacular Voice Parsing (20+ phrases)..." << std::endl;
    VoiceKioskAdapter adapter(0.80);

    struct TestCase {
        std::string phrase;
        bool expectParsed;
        CommodityType expectedCrop;
        double expectedQuintals;
        bool expectHighConfidence;
    };

    std::vector<TestCase> cases = {
        // 1-5: Clean standard Hindi/Hinglish
        {"50 crates tamatar bechna hai", true, CommodityType::Tomato, 12.5, true},     // 50 * 0.25 = 12.5q
        {"so peti tamatar bechna", true, CommodityType::Tomato, 25.0, true},           // 100 * 0.25 = 25q
        {"pachaas quintal pyaaz mandi bhejo", true, CommodityType::Onion, 50.0, true}, // 50q
        {"10 quintal mirch becho", true, CommodityType::Chilli, 10.0, true},           // 10q
        {"tees peti mirchi bechna", true, CommodityType::Chilli, 7.5, true},           // 30 * 0.25 = 7.5q

        // 6-10: Variations in vocabulary (kanda, kattal, english words)
        {"ek sau quintal kanda bechna hai", true, CommodityType::Onion, 100.0, true},
        {"25 crates tamatar dispatch karo", true, CommodityType::Tomato, 6.25, true},
        {"paanch quintal pyaaz bikri", true, CommodityType::Onion, 5.0, true},
        {"chaalis peti mirch bhejna", true, CommodityType::Chilli, 10.0, true},
        {"15 quintals tomato sell", true, CommodityType::Tomato, 15.0, true},

        // 11-15: Numeric word variations (dosau, dedhsau, dhaisau, kg)
        {"dosau peti tamatar", true, CommodityType::Tomato, 50.0, true},
        {"das quintal onion", true, CommodityType::Onion, 10.0, true},
        {"dedhsau peti kanda", true, CommodityType::Onion, 37.5, true},
        {"dhaisau peti tamatar bechna", true, CommodityType::Tomato, 62.5, true},
        {"500 kg tamatar bechna", true, CommodityType::Tomato, 5.0, true}, // 500 kg = 5q

        // 16-18: Noisy / conversational sentences
        {"bhai 50 crates tamatar jaldi se mandi me becho na please", true, CommodityType::Tomato, 12.5, false},
        {"are yaar 20 quintal pyaaz bechna hai urgently", true, CommodityType::Onion, 20.0, false},
        {"arre bhai jaldi se 10 quintal mirch mandi dispatch karo", true, CommodityType::Chilli, 10.0, false},

        // 19-22: Ambiguous or emergency phrases
        {"rok do truck cancel karo", true, CommodityType::Tomato, 0.0, false}, // Emergency stop
        {"band karo sauda", true, CommodityType::Tomato, 0.0, false},
        {"sirf 5 peti mirchi", true, CommodityType::Chilli, 1.25, false},      // Missing intent -> lower confidence
        {"kuch bhi becho na", false, CommodityType::Tomato, 0.0, false}        // No quantity, no crop
    };

    assert(cases.size() >= 20);

    int testIdx = 1;
    for (const auto& tc : cases) {
        InputEvent event;
        event.type = InputEventType::VoiceText;
        event.payload = tc.phrase;
        event.sourceFarmer = AgentId("Farmer-Test");

        auto result = adapter.parse(event);

        if (tc.expectParsed && tc.expectedQuintals > 0.0) {
            assert(result.command.has_value());
            auto* createCmd = std::get_if<CreateHarvestBatchCommand>(&result.command.value());
            assert(createCmd != nullptr);
            assert(createCmd->commodity == tc.expectedCrop);
            assert(std::abs(createCmd->quantity.value - tc.expectedQuintals) < 1e-3);
        }

        // Per hard rule: All voice orders generate ConfirmationCard
        if (result.command.has_value()) {
            assert(result.confirmationCard.has_value());
            assert(result.confirmationCard->requiresOneTapAction);
        }

        if (tc.expectHighConfidence) {
            assert(result.confidence >= adapter.confidenceThreshold());
        }
        testIdx++;
    }

    std::cout << "  [PASS] All 22 vernacular voice phrases parsed successfully." << std::endl;
}

void testButtonTapAndDtmf() {
    std::cout << "[RUN] Testing One-Click Button Tap and DTMF Inputs..." << std::endl;
    VoiceKioskAdapter adapter;

    // 1. One-click green tick confirmation
    InputEvent greenTick;
    greenTick.type = InputEventType::ButtonTap;
    greenTick.payload = "GREEN_TICK";
    greenTick.sourceFarmer = AgentId("Farmer-01");

    auto res1 = adapter.parse(greenTick);
    assert(res1.confidence == 1.0);
    assert(res1.command.has_value());
    auto* confirmCmd = std::get_if<ConfirmContractCommand>(&res1.command.value());
    assert(confirmCmd != nullptr);
    assert(confirmCmd->confirmed);

    // 2. Red cross cancel
    InputEvent redCross;
    redCross.type = InputEventType::ButtonTap;
    redCross.payload = "RED_CROSS";
    redCross.sourceFarmer = AgentId("Farmer-01");

    auto res2 = adapter.parse(redCross);
    assert(res2.command.has_value());
    auto* rejectCmd = std::get_if<RejectContractCommand>(&res2.command.value());
    assert(rejectCmd != nullptr);

    // 3. DTMF 1 -> Confirm
    InputEvent dtmf1;
    dtmf1.type = InputEventType::Dtmf;
    dtmf1.payload = "1";
    dtmf1.sourceFarmer = AgentId("Farmer-01");

    auto res3 = adapter.parse(dtmf1);
    assert(res3.command.has_value());
    assert(std::holds_alternative<ConfirmContractCommand>(res3.command.value()));

    // 4. DTMF 2 -> Reject
    InputEvent dtmf2;
    dtmf2.type = InputEventType::Dtmf;
    dtmf2.payload = "2";
    dtmf2.sourceFarmer = AgentId("Farmer-01");

    auto res4 = adapter.parse(dtmf2);
    assert(res4.command.has_value());
    assert(std::holds_alternative<RejectContractCommand>(res4.command.value()));

    std::cout << "  [PASS] Button taps and DTMF keys emitted typed domain commands." << std::endl;
}

void testProduceStatusCard() {
    std::cout << "[RUN] Testing ProduceStatusCard (Green / Amber / Red thresholds)..." << std::endl;

    // Green: Q >= 0.85
    auto cardGreen = ProduceStatusCard::fromQuality(CommodityType::Tomato, 0.92);
    assert(cardGreen.color == ProduceCardColor::Green);
    assert(cardGreen.grade == CropGrade::GradeA);
    assert(cardGreen.title.find("Grade A") != std::string::npos);

    // Amber: 0.65 <= Q < 0.85 (Grade B)
    auto cardAmberB = ProduceStatusCard::fromQuality(CommodityType::Tomato, 0.72);
    assert(cardAmberB.color == ProduceCardColor::Amber);
    assert(cardAmberB.grade == CropGrade::GradeB);

    // Amber: 0.40 <= Q < 0.65 (Grade C)
    auto cardAmberC = ProduceStatusCard::fromQuality(CommodityType::Tomato, 0.52);
    assert(cardAmberC.color == ProduceCardColor::Amber);
    assert(cardAmberC.grade == CropGrade::GradeC);

    // Red: Q < 0.40 (Spoiled)
    auto cardRed = ProduceStatusCard::fromQuality(CommodityType::Tomato, 0.32);
    assert(cardRed.color == ProduceCardColor::Red);
    assert(cardRed.grade == CropGrade::Spoiled);
    assert(cardRed.adviceHinglish.find("Processing factory") != std::string::npos);

    std::cout << "  [PASS] ProduceStatusCard accurately classified Grade A, B, C, and Spoiled." << std::endl;
}

void testAudioSpeakerNotifier() {
    std::cout << "[RUN] Testing AudioSpeakerNotifier Hinglish speech announcements..." << std::endl;
    AudioSpeakerNotifier notifier(false); // Disable direct console output in test

    AnomalyAlert alertStall;
    alertStall.type = AnomalyType::HighwayStoppage;
    alertStall.description = "NH44 stoppage";
    notifier.onAnomalyAlert(alertStall);

    AnomalyAlert alertGlut;
    alertGlut.type = AnomalyType::MandiGlut;
    alertGlut.description = "Azadpur price crash";
    notifier.onAnomalyAlert(alertGlut);

    AnomalyAlert alertFailure;
    alertFailure.type = AnomalyType::TransporterFailure;
    alertFailure.description = "Engine failure";
    notifier.onAnomalyAlert(alertFailure);

    const auto& history = notifier.transcriptHistory();
    assert(history.size() == 3);
    assert(history[0].find("highway jam") != std::string::npos);
    assert(history[1].find("bhaav gir gaya") != std::string::npos);
    assert(history[2].find("engine kharab") != std::string::npos);

    std::cout << "  [PASS] AudioSpeakerNotifier generated natural Hinglish voice alerts." << std::endl;
}

void testDashboardPlainModeSnapshot() {
    std::cout << "[RUN] Testing Dashboard Plain-Mode Snapshot..." << std::endl;
    Dashboard dashboard(DashboardMode::Plain);
    dashboard.setSimClock(SimTime(240), 42);

    // Update truck lanes
    TruckLaneView t1;
    t1.truckId = AgentId("Truck-DL01");
    t1.route = "NH44->Azadpur";
    t1.progressPercent = 50.0;
    t1.state = TruckState::EN_ROUTE;
    t1.cargoSummary = "50q Tomato";
    t1.eta = SimTime(480);
    t1.quality = 0.88;
    dashboard.updateTruck(t1);

    // Update mandi tickers
    MandiTickerView m1;
    m1.mandiId = AgentId("Azadpur");
    m1.name = "Azadpur";
    m1.currentPrice = Money::fromRupees(2200);
    m1.previousPrice = Money::fromRupees(2100);
    m1.glutFlag = false;
    m1.occupiedSlots = 3;
    m1.capacity = 4;
    dashboard.updateMandi(m1);

    MandiTickerView m2;
    m2.mandiId = AgentId("Ghazipur");
    m2.name = "Ghazipur";
    m2.currentPrice = Money::fromRupees(1320);
    m2.previousPrice = Money::fromRupees(2200);
    m2.glutFlag = true;
    m2.occupiedSlots = 4;
    m2.capacity = 4;
    dashboard.updateMandi(m2);

    // Add event log entries
    EventLog::instance().clear();
    EventLog::instance().append(SimTime(180), EventLevel::FAULT, "WATCHDOG", "Highway stoppage detected on NH44");
    EventLog::instance().append(SimTime(185), EventLevel::RECOVERY, "RECOVERY", "Diverted to Tropicana Processing Plant");

    std::string output = dashboard.render();

    // Plain mode verification: Zero ANSI escape sequences
    assert(output.find("\033[") == std::string::npos);

    // Verify all 5 panels are present
    assert(output.find("KISANSETU (MANDIFLOW)") != std::string::npos);
    assert(output.find("[PANEL 1: TRUCK TRANSIT LANES]") != std::string::npos);
    assert(output.find("[PANEL 2: CROP FRESHNESS METERS]") != std::string::npos);
    assert(output.find("[PANEL 3: MANDI PRICE TICKERS & GATE SLOTS]") != std::string::npos);
    assert(output.find("[PANEL 4: RECOVERY & EVENT LOG") != std::string::npos);

    // Verify content snapshot
    assert(output.find("Truck-DL01") != std::string::npos);
    assert(output.find("50%") != std::string::npos);
    assert(output.find("Q = 0.880") != std::string::npos);
    assert(output.find("Azadpur") != std::string::npos);
    assert(output.find("[GLUT ALERT!]") != std::string::npos);
    assert(output.find("[3/4 occupied]") != std::string::npos);
    assert(output.find("Highway stoppage detected on NH44") != std::string::npos);

    std::cout << "  [PASS] Dashboard Plain-Mode snapshot verified with zero ANSI leaks." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "KisanSetu (MandiFlow) - Phase B3 Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    testVernacularVoiceParsing20PlusPhrases();
    testButtonTapAndDtmf();
    testProduceStatusCard();
    testAudioSpeakerNotifier();
    testDashboardPlainModeSnapshot();

    std::cout << "========================================" << std::endl;
    std::cout << "All Phase B3 Interface & Dashboard Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
