#pragma once
#include <cstdint>
#include <limits>
#include <optional>

namespace touchcard {
enum class Medium : uint8_t { Internal, Sd };
enum class MediaState : uint8_t { Absent, Loading, Ready, ReadOnly, Error, Ejected };
enum class Destination : uint8_t { Auto, InternalOnly };
enum class UpdateTarget : uint8_t { Profile, Avatar, Dashboard, Fullscreen, Clock, ReceivedCard };

struct Usage {
  MediaState state = MediaState::Absent;
  uint64_t total = 0, used = 0, reserve = 0;
  bool valid() const { return total > 0 && used <= total; }
  uint64_t free() const { return valid() ? total - used : 0; }
  uint64_t available() const { return free() > reserve ? free() - reserve : 0; }
  bool fits(uint64_t bytes) const {
    return state == MediaState::Ready && valid() && bytes > 0 && bytes <= available();
  }
};

// nullopt means overflow, never a small wrapped allocation.
inline std::optional<uint64_t> requiredBytes(uint64_t payload, uint64_t index,
                                             uint64_t workspace) {
  const auto max = std::numeric_limits<uint64_t>::max();
  if (index > max - payload || workspace > max - payload - index) return {};
  return payload + index + workspace;
}

inline std::optional<Medium> chooseDestination(Destination preference, uint64_t bytes,
    const Usage& internal, const Usage& sd) {
  if (preference == Destination::InternalOnly)
    return internal.fits(bytes) ? std::optional<Medium>(Medium::Internal) : std::nullopt;
  if (sd.fits(bytes)) return Medium::Sd;
  if (internal.fits(bytes)) return Medium::Internal;
  return {};
}

// A transfer may not continue on another mount, even if it uses the same slot.
struct TransferLease {
  Medium medium;
  uint64_t volumeId, mountGeneration;
  bool matches(Medium candidate, uint64_t id, uint64_t generation, MediaState state) const {
    return state == MediaState::Ready && medium == candidate && volumeId == id &&
           mountGeneration == generation;
  }
};
inline bool targetAllowed(bool phoneUpdateSession, UpdateTarget target) {
  return phoneUpdateSession ? target != UpdateTarget::ReceivedCard
                            : target == UpdateTarget::ReceivedCard;
}
}  // namespace touchcard
