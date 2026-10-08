#include "starfield/core/ParticleBirth.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace {
using namespace starfield::core;
int checks{},failures{};
void check(bool value,const char* label) {++checks;if(!value){++failures;std::printf("FAILED: %s\n",label);}}
}
int main() {
    ParticleBirthControls defaults;
    check(defaults.seed_shift==0 && defaults.chance_percent==100,"reference defaults");
    check(valid_particle_birth_controls(defaults),"valid defaults");
    check(shifted_particle_seed(0,0)==0 && shifted_particle_seed(0xffffffffu,0)==0xffffffffu,"zero shift preserves all seed bits");
    check(shifted_particle_seed(0,-1)==0xffffffffu && shifted_particle_seed(0xffffffffu,1)==0,"modular positive and negative wrap");
    check(shifted_particle_seed(0x7fffffffu,std::numeric_limits<std::int32_t>::min())==0xffffffffu,"signed minimum shift is defined");
    check(shifted_particle_seed(0x80000000u,std::numeric_limits<std::int32_t>::max())==0xffffffffu,"signed maximum shift is defined");
    check(static_cast<std::uint64_t>(RandomPurpose::particle_birth_chance)==21 &&
        static_cast<std::uint64_t>(RandomPurpose::particle_cloud)==20 &&
        static_cast<std::uint64_t>(RandomPurpose::auxiliary_chance)==12,"append random purpose without renumbering old streams");
    for(double invalid:{-1.0,100.001,std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
        check(!valid_particle_birth_controls({0,invalid}) && !particle_birth_allowed({0,invalid},7,3),"invalid probability rejected");
    }
    for(double valid:{0.0,0.0001,50.0,99.999,100.0})check(valid_particle_birth_controls({0,valid}),"finite probability boundaries");
    for(std::uint32_t seed:{0u,1u,0x7fffffffu,0xffffffffu}) {
        std::vector<bool> births;births.reserve(2048);
        unsigned count25{},count50{},count75{},changed{};
        for(std::uint64_t ordinal=0;ordinal<2048;++ordinal) {
            const auto identity=mix64(ordinal^0x831e9b15d07aac42ull);
            const auto original_size=stream_bits(seed,ordinal,RandomPurpose::size);
            const auto original_speed=stream_bits(seed,ordinal,RandomPurpose::emission_speed);
            const bool p25=particle_birth_allowed({0,25},seed,identity),p50=particle_birth_allowed({0,50},seed,identity),
                p75=particle_birth_allowed({0,75},seed,identity);
            births.push_back(p50);count25+=p25;count50+=p50;count75+=p75;
            changed+=p50!=particle_birth_allowed({-37,50},seed,identity);
            check(particle_birth_allowed(defaults,seed,identity) && !particle_birth_allowed({0,0},seed,identity),"probability endpoints preserve all or no births");
            check((!p25 || p50) && (!p50 || p75),"increasing chance gives nested subsets without renumbering");
            check(p50==(unit_value(seed,identity,RandomPurpose::particle_birth_chance)<.5),"uses dedicated deterministic threshold");
            check(original_size==stream_bits(seed,ordinal,RandomPurpose::size) &&
                original_speed==stream_bits(seed,ordinal,RandomPurpose::emission_speed),"birth decisions do not consume or change released random streams");
        }
        for(std::uint64_t ordinal=2048;ordinal-->0;)
            check(births[ordinal]==particle_birth_allowed({0,50},seed,mix64(ordinal^0x831e9b15d07aac42ull)),"reverse evaluation has identical source membership");
        check(count25>410 && count25<620 && count50>900 && count50<1150 && count75>1430 && count75<1650,"probability distribution over fixed identity sample");
        check(changed>800 && changed<1250,"shift selects a different deterministic subset");
    }
    std::printf("particle_birth_policy_tests: %d checks, %d failures; evaluator/native/CEP integration remains pending\n",checks,failures);
    return failures?1:0;
}
