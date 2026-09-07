#include "StoragePolicy.h"
#include <cassert>
#include <iostream>
using namespace touchcard;
int main() {
  const Usage internal{MediaState::Ready, 8000000, 1000000, 1000000};
  Usage sd{MediaState::Ready, 64000000000ULL, 10000000000ULL, 4096};
  assert(sd.free() == 54000000000ULL);
  assert(internal.available() == 6000000);
  assert(chooseDestination(Destination::Auto, 100, internal, sd) == Medium::Sd);
  assert(chooseDestination(Destination::InternalOnly, 100, internal, sd) == Medium::Internal);
  for (auto state : {MediaState::Absent, MediaState::Loading, MediaState::ReadOnly,
                     MediaState::Error, MediaState::Ejected}) {
    sd.state = state;
    assert(chooseDestination(Destination::Auto, 100, internal, sd) == Medium::Internal);
    assert(!chooseDestination(Destination::Auto, 6000001, internal, sd));
  }
  assert(!chooseDestination(Destination::Auto, 0, internal, sd));
  assert(internal.fits(6000000));
  assert(!internal.fits(6000001));
  assert((Usage{MediaState::Ready, 1, 2, 0}.available() == 0));
  assert((Usage{MediaState::Ready, 100, 50, 60}.available() == 0));
  assert(requiredBytes(100, 20, 30) == 150);
  assert(!requiredBytes(UINT64_MAX, 1, 0));
  assert(!requiredBytes(UINT64_MAX - 1, 1, 1));
  TransferLease lease{Medium::Sd, 41, 2};
  assert(lease.matches(Medium::Sd, 41, 2, MediaState::Ready));
  assert(!lease.matches(Medium::Internal, 41, 2, MediaState::Ready));
  assert(!lease.matches(Medium::Sd, 42, 2, MediaState::Ready));
  assert(!lease.matches(Medium::Sd, 41, 3, MediaState::Ready));
  assert(!lease.matches(Medium::Sd, 41, 2, MediaState::Ejected));
  for (auto target : {UpdateTarget::Profile, UpdateTarget::Avatar, UpdateTarget::Dashboard,
                     UpdateTarget::Fullscreen, UpdateTarget::Clock}) {
    assert(targetAllowed(true, target));
    assert(!targetAllowed(false, target));
  }
  assert(!targetAllowed(true, UpdateTarget::ReceivedCard));
  assert(targetAllowed(false, UpdateTarget::ReceivedCard));
  std::cout << "storage policy: PASS\n";
}
