#include "strategy/IPricingStrategy.h"
#include "domain/PriceModel.h"

namespace kisansetu::strategy {

namespace {

class SurgePricingStrategy final : public IPricingStrategy {
public:
    explicit SurgePricingStrategy(double sensitivityK = 0.50, double floorMultiplier = 0.40)
        : sensitivityK_(sensitivityK), floorMultiplier_(floorMultiplier) {}

    [[nodiscard]] contracts::Money quotePrice(
        const domain::Commodity& commodity,
        contracts::SimTime /*slotTime*/,
        size_t currentQueueLength) const override {
        // Queue length relative to hourly gate capacity (nominal 4 trucks/hour)
        return domain::PriceModel::calculateSurgePrice(
            commodity.basePricePerQuintal,
            static_cast<double>(currentQueueLength),
            4.0, // nominal absorption capacity
            sensitivityK_,
            floorMultiplier_);
    }

    [[nodiscard]] contracts::Money clearAuction(
        contracts::Money basePrice,
        size_t totalArrivals,
        double absorptionCapacity) const override {
        return domain::PriceModel::calculateSurgePrice(
            basePrice,
            static_cast<double>(totalArrivals),
            absorptionCapacity,
            sensitivityK_,
            floorMultiplier_);
    }

private:
    double sensitivityK_{0.50};
    double floorMultiplier_{0.40};
};

class UniformPriceAuctionStrategy final : public IPricingStrategy {
public:
    explicit UniformPriceAuctionStrategy(double sensitivityK = 0.50, double floorMultiplier = 0.40)
        : sensitivityK_(sensitivityK), floorMultiplier_(floorMultiplier) {}

    [[nodiscard]] contracts::Money quotePrice(
        const domain::Commodity& commodity,
        contracts::SimTime /*slotTime*/,
        size_t currentQueueLength) const override {
        return domain::PriceModel::calculateSurgePrice(
            commodity.basePricePerQuintal,
            static_cast<double>(currentQueueLength),
            4.0,
            sensitivityK_,
            floorMultiplier_);
    }

    [[nodiscard]] contracts::Money clearAuction(
        contracts::Money basePrice,
        size_t totalArrivals,
        double absorptionCapacity) const override {
        // Uniform call auction clearing: all cleared batches receive the same uniform price
        return domain::PriceModel::calculateSurgePrice(
            basePrice,
            static_cast<double>(totalArrivals),
            absorptionCapacity,
            sensitivityK_,
            floorMultiplier_);
    }

private:
    double sensitivityK_{0.50};
    double floorMultiplier_{0.40};
};

} // namespace

std::unique_ptr<IPricingStrategy> createSurgePricingStrategy(double sensitivityK, double floorMultiplier) {
    return std::make_unique<SurgePricingStrategy>(sensitivityK, floorMultiplier);
}

std::unique_ptr<IPricingStrategy> createUniformPriceAuctionStrategy(double sensitivityK, double floorMultiplier) {
    return std::make_unique<UniformPriceAuctionStrategy>(sensitivityK, floorMultiplier);
}

} // namespace kisansetu::strategy
