#pragma once
#ifndef _PARTICLE_DEF_TRAITS_HPP_
#define _PARTICLE_DEF_TRAITS_HPP_
#include <type_traits>

namespace engine {

template <typename ParticleType> struct remove_cvref {
  using type = std::remove_cv_t<std::remove_reference_t<ParticleType>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename ParticleType>
using __Particle_Trait =
    std::void_t<decltype(std::declval<remove_cvref_t<ParticleType>>().position),
                decltype(std::declval<remove_cvref_t<ParticleType>>().velocity),
                decltype(std::declval<remove_cvref_t<ParticleType>>().color)>;

template <typename T, typename = void>
struct has_particle_fields : std::false_type {};

template <typename T>
struct has_particle_fields<T, __Particle_Trait<T>> : std::true_type {};

template <typename T>
static constexpr bool has_particle_fields_v = has_particle_fields<T>::value;

template <typename ParticleT, typename = void> struct ParticleLayoutTraits;

template <typename ParticleT>
struct ParticleLayoutTraits<
    ParticleT, std::enable_if_t<has_particle_fields_v<ParticleT>>> {
  static constexpr bool has_position = true;
  static constexpr bool has_velocity = true;
  static constexpr bool has_color = true;

  static constexpr std::size_t aos_stride = sizeof(ParticleT);

  static constexpr std::size_t position_offset = offsetof(ParticleT, position);
  static constexpr std::size_t velocity_offset = offsetof(ParticleT, velocity);
  static constexpr std::size_t color_offset = offsetof(ParticleT, color);

  static constexpr std::size_t position_size =
      sizeof(decltype(std::declval<ParticleT>().position));
  static constexpr std::size_t velocity_size =
      sizeof(decltype(std::declval<ParticleT>().velocity));
  static constexpr std::size_t color_size =
      sizeof(decltype(std::declval<ParticleT>().color));
};

template <typename ParticleT, template <typename, typename> class LayoutT,
          typename = void>
struct is_valid_particle_layout : std::false_type {};

template <typename ParticleT, template <typename, typename> class LayoutT>
struct is_valid_particle_layout<
    ParticleT, LayoutT, std::void_t<typename LayoutT<ParticleT, void>::view>>
    : std::bool_constant<has_particle_fields_v<ParticleT>> {};

template <typename ParticleT, template <typename, typename> class LayoutT>
static constexpr bool is_valid_particle_layout_v =
    is_valid_particle_layout<ParticleT, LayoutT>::value;
} // namespace engine

#endif //_PARTICLE_DEF_TRAITS_HPP_
