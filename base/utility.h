#ifndef BASE_UTILITY_H_
#define BASE_UTILITY_H_

namespace cos {
namespace base {


struct normal {};
struct urgent {};
struct sequence {};

#if __cplusplus >= 201703L
template <typename F, typename... Args>
using result_of_t = std::invoke_result_t<F, Args...>;
#else
template <typename F, typename...Args>
using result_of_t = std::result_of_t<F, Args...>;
#endif


}  // namespace base
}  // namespace cos

#endif  // BASE_UTILITY_H_