#pragma once
#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <array>
#include <cstdint>
#include <span>

namespace starfield::adapter {
inline constexpr double transaction_backup_guard=2;
// UI callback scope only. Caller owns one outer undo group and releases all
// effect/stream references before calling prepare/restore or ExecuteScript.
class EffectGraphBackup final {
public:
    EffectGraphBackup(SPBasicSuite*,AEGP_PluginID,AEGP_LayerH) noexcept;
    ~EffectGraphBackup();
    EffectGraphBackup(const EffectGraphBackup&)=delete;
    EffectGraphBackup& operator=(const EffectGraphBackup&)=delete;
    A_Err prepare(const std::array<std::uint8_t,16>& transaction_id,
        std::span<const std::array<std::uint8_t,16>> desired_ids={}) noexcept;
    A_Err restore() noexcept;
    // After publication, deletion failures leave marked, disabled backups and
    // must be reported as cleanup failures, not as an unapplied graph edit.
    A_Err discard() noexcept;
    A_Err rollback_error() const noexcept {return rollback_error_;}
private:
    struct Record {
        A_long kind{-1},index{},guard{},uuid_first{};
        AEGP_EffectFlags flags{};
        std::array<std::uint8_t,16> original{},backup{};
        std::array<A_UTF16Char,512> name{};
        bool created{};
    };
    SPBasicSuite* basic_{};AEGP_PluginID plugin_{};AEGP_LayerH layer_{};
    const AEGP_EffectSuite4* effects_{};const AEGP_StreamSuite6* streams_{};
    const AEGP_DynamicStreamSuite4* dynamic_{};const AEGP_MemorySuite1* memory_{};
    std::array<Record,64> records_{};std::size_t count_{};
    bool prepared_{};A_Err rollback_error_{};
    A_Err scalar(AEGP_EffectRefH,A_long,double&) noexcept;
    A_Err write_scalar(AEGP_EffectRefH,A_long,double) noexcept;
    A_Err describe(AEGP_EffectRefH,Record&,bool&) noexcept;
    A_Err identity(AEGP_EffectRefH,const Record&,std::array<std::uint8_t,16>&) noexcept;
    A_Err write_identity(AEGP_EffectRefH,const Record&,const std::array<std::uint8_t,16>&) noexcept;
    A_Err find(const Record&,bool,AEGP_EffectRefH&,A_long&) noexcept;
    A_Err group(A_long,AEGP_StreamRefH&) noexcept;
    A_Err name(A_long,std::array<A_UTF16Char,512>&) noexcept;
    A_Err set_name(A_long,const std::array<A_UTF16Char,512>&) noexcept;
    A_Err erase(AEGP_EffectRefH) noexcept;
    A_Err release_guard() noexcept;
    A_Err delete_backups() noexcept;
};
}
