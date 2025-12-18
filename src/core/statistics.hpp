#pragma once
#include <cstddef>
#include <string>
#include <vector>

struct StatisticsSummary {
  std::size_t count = 0;
  double sum = 0.0;
  double mean = 0.0;
  double median = 0.0;
  double minimum = 0.0;
  double maximum = 0.0;
  double range = 0.0;
  double variance = 0.0;
  double standardDeviation = 0.0;
  std::vector<double> modes;
};

StatisticsSummary calculateStatistics(const std::vector<double> &values);
double calculatePercentile(const std::vector<double> &values,
                           double percentile);
std::vector<std::string> buildAsciiGraph(const std::vector<double> &values,
                                         std::size_t height);
