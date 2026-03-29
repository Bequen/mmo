#pragma once

// #include <fmt/base.h>
// #include <format>
// #include <string>
// #include <algorithm>
// #include <type_traits>
// #include <spdlog/spdlog.h>
//
// template<typename T>
// class Err;
//
//
// template<typename T>
// class Ok {
//     using OkDataType = std::conditional_t<std::is_same_v<T, void>, int, T>;
//
//     OkDataType value;
//
//     public:
//     explicit constexpr Ok() { }
//
//     // Constructor from value
//     template<typename U = T,
//              typename = std::enable_if_t<!std::is_void_v<U>>>
//     explicit constexpr Ok(const U&& val) {
//         value = std::move(val);
//     }
//
//     // template<typename TErr>
//     // constexpr operator Result<T, TErr>() const & {
//     //     return Result<T, TErr>(std::move(value));
//     // }
//     //
//     // template<typename TErr>
//     // constexpr operator Result<T, TErr>() && {
//     //     return Result<T, TErr>(value);
//     // }
// };
//
// Ok() -> Ok<int>;
//
// template<typename A>
// Ok(A) -> Ok<A>;
//
//
// template<typename T>
// class Err {
// public:
//     T value;
//
//     public:
//     explicit constexpr Err(T&& val) : value{val} { }
//
//     // template<typename TErr>
//     // constexpr operator Result<TErr, std::decay_t<T>>() const & {
//     // }
//     //
//     // template<typename TErr>
//     // constexpr operator Result<TErr, std::decay_t<T>>() && {
//     // }
//
//
// };
//
//
// template<typename A>
// Err(A) -> Err<A>;
//
// template <typename T, typename TErr>
// union Result {
// private:
//     using OkDataType = std::conditional_t<std::is_same_v<T, void>, int, T>;
//
//     struct {
//         bool is_ok;
//         Ok<OkDataType> value;
//     } _ok;
//
//     struct {
//         bool is_ok;
//         Err<TErr> error;
//     } _err;
//
// public:
//     constexpr bool is_ok() {
//         return _ok.is_ok;
//     }
//
//     constexpr OkDataType value() {
//         return _ok.value;
//     }
//
//     Result(const Ok<OkDataType>& value) {
//         _ok.is_ok = true;
//         _ok.value = std::move(value);
//     }
//
//
//     Result(const Err<TErr>& value) {
//         _err.is_ok = false;
//         _err.error = std::move(value);
//
//         if constexpr (fmt::is_formattable<TErr>()) {
//             spdlog::error("{}", _err.error.value);
//         }
//     }
//
//     // constexpr Result(const OkDataType& value) {
//     //     _ok.is_ok = true;
//     //     _ok.value = std::move(value);
//     // }
//     //
//     // constexpr Result(const std::decay_t<TErr>& value) {
//     //     _ok.is_ok = false;
//     //     _ok.value = std::move(value);
//     // }
//
//     ~Result() {}
//
//     void expect(const std::string& message) {
//         if(!is_ok()) {
//             throw std::runtime_error(std::format("Result failed: {}", message));
//         }
//     }
// };
//
// #include <algorithm>
// #include <spdlog/spdlog.h>
// #include <variant>
// template <typename T = void>
// class Ok {
//     using OkDataType = std::conditional_t<std::is_same_v<T, void>, int, T>;

//   public:

//     template<typename U = T,
//              typename = std::enable_if_t<std::is_void_v<U>>>
//     explicit constexpr Ok() { }

//     explicit constexpr Ok(OkDataType&& val) : value(std::move(val)) { }

//     constexpr OkDataType&& take_value() { return std::move(value); }

//     OkDataType value;
// };

// Ok() -> Ok<int>;

// template<typename A>
// Ok(A&&) -> Ok<A>;

// template <typename T>
// class Err {

//   public:
//     explicit constexpr Err(T value) : value(std::move(value)) {
//         // if constexpr (fmt::is_formattable<T>()) {
//         //     spdlog::error("Error: {}", value);
//         // } else {
//         //     spdlog::error("Error");
//         // }
//     }

//     constexpr T&& take_value() { return std::move(value); }

//     T value;
// };

// template <typename OkT, typename ErrT>
// class Result {
//     using OkDataType = std::conditional_t<std::is_same_v<OkT, void>, int, OkT>;
//   public:
//     using VariantT = std::variant<Ok<OkT>, Err<ErrT>>;

//     constexpr Result(Ok<OkT> value) : variant(std::move(value)) {}
//     constexpr Result(Err<ErrT> value) : variant(std::move(value)) {}

//     constexpr bool is_ok() const { return std::holds_alternative<Ok<OkT>>(variant); }
//     constexpr bool is_err() const { return std::holds_alternative<Err<ErrT>>(variant); }

//     constexpr OkDataType ok_value() const { return std::get<Ok<OkT>>(variant).value; }
//     constexpr ErrT err_value() const { return std::get<Err<ErrT>>(variant).value; }

//     constexpr OkDataType&& take_ok_value() { return std::get<Ok<OkT>>(variant).take_value(); }
//     constexpr ErrT&& take_err_value() { return std::get<Err<ErrT>>(variant).take_value(); }

//     VariantT variant;
// };
