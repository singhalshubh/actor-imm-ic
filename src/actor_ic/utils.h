static const int64_t kRandSeed = 27491095;

#define ASSERT_WITH_MESSAGE(condition, message)\
   (!(condition)) ?\
      (std::cerr << "Assertion failed: (" << #condition << "), "\
      << "function " << __FUNCTION__\
      << ", file " << __FILE__\
      << ", line " << __LINE__ << "."\
      << std::endl << message << std::endl, abort(), 0) : 1

inline double logBinomial(size_t n, size_t k) {
  return n * log(n) - k * log(k) - (n - k) * log(n - k);
}

inline double generateRandomNumber(uint64_t upper_limit) {
    return (double)rand() / RAND_MAX;
}

ssize_t ThetaPrime(ssize_t x, double epsilonPrime, double l, size_t k, size_t num_nodes) {
  k = std::min(k, num_nodes/2);
  return (2 + 2. / 3. * epsilonPrime) *
         (l * std::log(num_nodes) + logBinomial(num_nodes, k) +
          std::log(std::log2(num_nodes))) *
         std::pow(2.0, x) / (epsilonPrime * epsilonPrime);
}

inline size_t Theta(double epsilon, double l, size_t k, double LB,
                    size_t num_nodes) {
  if (LB == 0) return 0;

  k = std::min(k, num_nodes/2);
  double term1 = 0.6321205588285577;  // 1 - 1/e
  double alpha = sqrt(l * std::log(num_nodes) + std::log(2));
  double beta = sqrt(term1 * (logBinomial(num_nodes, k) +
                              l * std::log(num_nodes) + std::log(2)));
  double lamdaStar = 2 * num_nodes * (term1 * alpha + beta) *
                     (term1 * alpha + beta) * pow(epsilon, -2);
  return lamdaStar / LB;
}

