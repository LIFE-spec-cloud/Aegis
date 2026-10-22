#pragma once

#include "contracts/Types.h"
#include <string>
#include <sstream>
#include <iomanip>

namespace kisansetu::domain {

/**
 * @brief Performance metrics for a single simulation run.
 */
struct SimulationRunMetrics {
    std::string systemName;
    double totalHarvestVolumeQtl{0.0};
    double deliveredVolumeQtl{0.0};
    double spoiledVolumeQtl{0.0};
    double salvagedVolumeQtl{0.0};
    contracts::Money totalGrossRevenue{0};
    contracts::Money totalFreightCost{0};
    contracts::Money totalNetMargin{0};
    double averageNetMarginPerQtl{0.0};
    double spoilageRatePercent{0.0};
    double salvageRatePercent{0.0};
};

/**
 * @brief End-to-end comparative evaluation between baseline and KisanSetu.
 */
struct BaselineComparisonResult {
    SimulationRunMetrics baseline;
    SimulationRunMetrics kisanSetu;
    double netMarginImprovementPercent{0.0};
    double spoilageReductionPercent{0.0};

    [[nodiscard]] std::string toReportString() const {
        std::ostringstream oss;
        oss << "\n========================================================================\n"
            << "       KisanSetu (MandiFlow) vs Static Baseline Evaluation Report       \n"
            << "========================================================================\n"
            << std::left << std::setw(28) << "Metric"
            << std::right << std::setw(20) << "Static Baseline"
            << std::setw(20) << "KisanSetu Full"
            << "\n------------------------------------------------------------------------\n"
            << std::left << std::setw(28) << "Total Harvest (qtl)"
            << std::right << std::setw(20) << std::fixed << std::setprecision(1) << baseline.totalHarvestVolumeQtl
            << std::setw(20) << kisanSetu.totalHarvestVolumeQtl << "\n"
            << std::left << std::setw(28) << "Delivered Produce (qtl)"
            << std::right << std::setw(20) << baseline.deliveredVolumeQtl
            << std::setw(20) << kisanSetu.deliveredVolumeQtl << "\n"
            << std::left << std::setw(28) << "Spoiled Produce (qtl)"
            << std::right << std::setw(20) << baseline.spoiledVolumeQtl
            << std::setw(20) << kisanSetu.spoiledVolumeQtl << "\n"
            << std::left << std::setw(28) << "Salvaged via Recovery (qtl)"
            << std::right << std::setw(20) << baseline.salvagedVolumeQtl
            << std::setw(20) << kisanSetu.salvagedVolumeQtl << "\n"
            << std::left << std::setw(28) << "Total Net Margin (INR)"
            << std::right << std::setw(20) << std::setprecision(2) << baseline.totalNetMargin.toRupees()
            << std::setw(20) << kisanSetu.totalNetMargin.toRupees() << "\n"
            << std::left << std::setw(28) << "Avg Net Margin (INR/qtl)"
            << std::right << std::setw(20) << baseline.averageNetMarginPerQtl
            << std::setw(20) << kisanSetu.averageNetMarginPerQtl << "\n"
            << std::left << std::setw(28) << "Produce Spoilage Rate (%)"
            << std::right << std::setw(19) << baseline.spoilageRatePercent << "%"
            << std::setw(19) << kisanSetu.spoilageRatePercent << "%\n"
            << std::left << std::setw(28) << "Salvage Rate (%)"
            << std::right << std::setw(19) << baseline.salvageRatePercent << "%"
            << std::setw(19) << kisanSetu.salvageRatePercent << "%\n"
            << "------------------------------------------------------------------------\n"
            << "  >>> Net Margin Improvement: +" << std::setprecision(1) << netMarginImprovementPercent << "%\n"
            << "  >>> Spoilage Reduction:     -" << std::setprecision(1) << spoilageReductionPercent << "%\n"
            << "========================================================================\n";
        return oss.str();
    }
};

/**
 * @brief Benchmark evaluator comparing uncoordinated static single-mandi runs
 * against KisanSetu's multi-agent dynamic coordination and recovery.
 */
class BaselineEvaluator {
public:
    /**
     * @brief Run calibrated benchmark comparison.
     * @param totalBatches Number of simulated harvest batches (e.g. 10).
     * @param batchVolumeQtl Volume per batch in quintals (e.g. 30 qtl).
     * @param failureRate Fraction of batches experiencing in-transit stalls or gluts (e.g. 0.30).
     */
    [[nodiscard]] static BaselineComparisonResult runEvaluation(
        size_t totalBatches = 10,
        double batchVolumeQtl = 30.0,
        double failureRate = 0.30);
};

} // namespace kisansetu::domain
