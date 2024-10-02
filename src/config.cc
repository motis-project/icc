#include "motis/config.h"

#include <iostream>

#include "fmt/std.h"

#include "utl/read_file.h"
#include "utl/verify.h"

#include "rfl.hpp"
#include "rfl/yaml.hpp"

namespace motis {

template <rfl::internal::StringLiteral Name, size_t I = 0, char... Chars>
consteval auto drop_last() {
  if constexpr (I == Name.arr_.size() - 2) {
    return rfl::internal::StringLiteral<sizeof...(Chars) + 1>(Chars...);
  } else {
    return drop_last<Name, I + 1, Chars..., Name.arr_[I]>();
  }
}

struct drop_trailing {
public:
  template <typename StructType>
  static auto process(auto&& named_tuple) {
    const auto handle_one = []<typename FieldType>(FieldType&& f) {
      if constexpr (FieldType::name() != "xml_content" &&
                    !rfl::internal::is_rename_v<typename FieldType::Type>) {
        return handle_one_field(std::move(f));
      } else {
        return std::move(f);
      }
    };
    return named_tuple.transform(handle_one);
  }

private:
  template <typename FieldType>
  static auto handle_one_field(FieldType&& _f) {
    using NewFieldType =
        rfl::Field<drop_last<FieldType::name_>(), typename FieldType::Type>;
    return NewFieldType(_f.value());
  }
};

template <rfl::internal::StringLiteral Name, size_t I = 0, char... Chars>
consteval auto drop_first() {
  if constexpr (I == 0U) {
    return drop_first<Name, I + 1, Chars...>();
  } else if (I == Name.arr_.size() - 1) {
    return rfl::internal::StringLiteral<sizeof...(Chars) + 1>(Chars...);
  } else {
    return drop_first<Name, I + 1, Chars..., Name.arr_[I]>();
  }
}

struct rename_enum {
public:
  template <typename StructType>
  static auto process(auto&& named_tuple) {
    const auto handle_one = []<typename FieldType>(FieldType&& f) {
      if constexpr (FieldType::name() != "xml_content" &&
                    std::is_enum_v<typename FieldType::Type>) {
        return handle_one_field(std::move(f));
      } else {
        return std::move(f);
      }
    };
    return named_tuple.transform(handle_one);
  }

private:
  template <typename FieldType>
  static auto handle_one_field(FieldType&& _f) {
    using NewFieldType =
        rfl::Field<drop_first<FieldType::name_>(), typename FieldType::Type>;
    return NewFieldType(_f.value());
  }
};

std::ostream& operator<<(std::ostream& out, config const& c) {
  return out << rfl::yaml::write<drop_trailing, rename_enum>(c);
}

config config::read(std::filesystem::path const& p) {
  auto const file_content = utl::read_file(p.generic_string().c_str());
  utl::verify(file_content.has_value(), "could not read config file at {}", p);
  return read(*file_content);
}

config config::read(std::string const& s) {
  return rfl::yaml::read<config, drop_trailing, rename_enum,
                         rfl::DefaultIfMissing>(s)
      .value();
}

}  // namespace motis