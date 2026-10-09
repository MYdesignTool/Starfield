#include "ModelControls.hpp"
#include "ModelGeometryParameter.hpp"
#include "AE_EffectCB.h"
#include "Param_Utils.h"
#include "starfield/core/ModelGeometry.hpp"
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <new>

namespace starfield::adapter {
namespace {
namespace layout=native_nodes::model_layout;
constexpr PF_ParamFlags editable=PF_ParamFlag_SUPERVISE;
constexpr PF_ParamFlags constant=editable|PF_ParamFlag_CANNOT_TIME_VARY;
constexpr PF_ParamUIFlags invisible=PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE;
core::NeverCancelled never;
PF_Err host_error(core::CoreError error) noexcept {
    return error.code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:
        error.code==core::ErrorCode::cancelled?PF_Interrupt_CANCEL:PF_Err_BAD_CALLBACK_PARAM;
}
PF_Err add(PF_InData* data,PF_ParamDef& def,int index) noexcept {
    def.uu.id=layout::disk_id(index);
    return PF_ADD_PARAM(data,-1,&def);
}
PF_Err float_slider(PF_InData* data,const char* name,int index,double limit,
    double drag_min,double drag_max,double initial,bool percent) noexcept {
    PF_ParamDef def{};def.param_type=PF_Param_FLOAT_SLIDER;def.flags=editable;
    std::snprintf(def.name,sizeof(def.name),"%s",name);
    def.u.fs_d.valid_min=static_cast<PF_FpShort>(-limit);
    def.u.fs_d.valid_max=static_cast<PF_FpShort>(limit);
    def.u.fs_d.slider_min=static_cast<PF_FpShort>(drag_min);
    def.u.fs_d.slider_max=static_cast<PF_FpShort>(drag_max);
    def.u.fs_d.value=initial;def.u.fs_d.dephault=static_cast<PF_FpShort>(initial);
    def.u.fs_d.precision=PF_Precision_HUNDREDTHS;
    def.u.fs_d.display_flags=static_cast<PF_ValueDisplayFlags>(percent?PF_ValueDisplayFlag_PERCENT:PF_ValueDisplayFlag_NONE);
    return add(data,def,index);
}
template<class T>core::Result<T> invalid(const char* detail) noexcept {
    return core::Result<T>::failure(core::ErrorCode::invalid_request,detail);
}
bool right_type(std::span<PF_ParamDef* const> params,int index,PF_ParamType type) noexcept {
    return index>0 && static_cast<std::size_t>(index)<params.size() && params[index] && params[index]->param_type==type;
}
void append_double(core::OpaqueBytes& bytes,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);
    for(unsigned b=0;b<8;++b)bytes.push_back(static_cast<std::byte>((bits>>(8*b))&255));
}
}

PF_Err register_model_author_controls(PF_InData* data) noexcept {
    if(!data || !data->inter.add_param || !data->utils || !data->utils->host_dispose_handle)
        return PF_Err_BAD_CALLBACK_PARAM;
    PF_ParamDef def{};def.param_type=PF_Param_POPUP;def.flags=constant;
    std::snprintf(def.name,sizeof(def.name),"Source");
    def.u.pd.num_choices=2;def.u.pd.value=def.u.pd.dephault=1;def.u.pd.u.namesptr="Cube|OBJ";
    auto error=add(data,def,layout::source);if(error)return error;
    def={};def.param_type=PF_Param_BUTTON;def.flags=constant;
    std::snprintf(def.name,sizeof(def.name),"Import OBJ");def.u.button_d.u.namesptr="Import OBJ...";
    error=add(data,def,layout::import_obj);if(error)return error;
    PF_ArbitraryH default_mesh=nullptr;
    const auto cube=core::make_unit_cube();if(!cube.has_value())return host_error(cube.error());
    error=create_model_geometry_parameter(data,cube.value(),&default_mesh,never);if(error)return error;
    def={};def.param_type=PF_Param_ARBITRARY_DATA;def.flags=PF_ParamFlag_NONE;def.ui_flags=invisible;
    std::snprintf(def.name,sizeof(def.name),"Model Mesh");
    def.u.arb_d.id=static_cast<A_short>(layout::disk_id(layout::mesh));def.u.arb_d.dephault=default_mesh;
    error=add(data,def,layout::mesh);
    if(error){data->utils->host_dispose_handle(default_mesh);return error;}
    // Accepted arbitrary defaults belong to the host, including a later setup
    // failure. Disposing one here would invalidate the host's stored definition.
    def={};def.param_type=PF_Param_SLIDER;def.flags=constant;def.ui_flags=invisible;
    std::snprintf(def.name,sizeof(def.name),"Mesh Revision");
    def.u.sd.valid_min=def.u.sd.slider_min=0;
    def.u.sd.valid_max=def.u.sd.slider_max=(std::numeric_limits<A_long>::max)();
    def.u.sd.value=def.u.sd.dephault=0;
    error=add(data,def,layout::revision);if(error)return error;
    constexpr const char* axes[]={"X","Y","Z"};
    for(int a=0;a<3;++a){char name[24]{};std::snprintf(name,sizeof(name),"Offset %s",axes[a]);
        error=float_slider(data,name,layout::origin+a,1e9,-1,1,0,false);if(error)return error;}
    for(int a=0;a<3;++a){def={};def.param_type=PF_Param_ANGLE;def.flags=editable|PF_ParamFlag_START_COLLAPSED;
        std::snprintf(def.name,sizeof(def.name),"Angle %s",axes[a]);def.u.ad.value=def.u.ad.dephault=0;
        error=add(data,def,layout::rotation+a);if(error)return error;}
    for(int a=0;a<3;++a){char name[24]{};std::snprintf(name,sizeof(name),"Scale %s",axes[a]);
        error=float_slider(data,name,layout::scale+a,100000,0,200,100,true);if(error)return error;}
    constexpr const char* booleans[]={"Flip X","Flip Y","Flip Z","Center","Normalize"};
    for(int b=0;b<5;++b){def={};def.param_type=PF_Param_CHECKBOX;def.flags=editable;
        std::snprintf(def.name,sizeof(def.name),"%s",booleans[b]);def.u.bd.u.nameptr=booleans[b];
        def.u.bd.value=def.u.bd.dephault=0;error=add(data,def,layout::flip_x+b);if(error)return error;}
    return PF_Err_NONE;
}

PF_Err register_model_author_bounds(PF_InData* data) noexcept {
    if(!data||!data->inter.add_param)return PF_Err_BAD_CALLBACK_PARAM;
    constexpr const char* names[]{"Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"};
    for(int axis=0;axis<6;++axis){PF_ParamDef def{};def.param_type=PF_Param_FLOAT_SLIDER;def.flags=constant;def.ui_flags=invisible;
        def.uu.id=layout::author_bounds_disk_id(layout::author_bounds_first+axis);
        std::snprintf(def.name,sizeof(def.name),"%s",names[axis]);
        def.u.fs_d.valid_min=def.u.fs_d.slider_min=-1e9f;def.u.fs_d.valid_max=def.u.fs_d.slider_max=1e9f;
        def.u.fs_d.value=def.u.fs_d.dephault=axis<3?-.5:.5;def.u.fs_d.precision=PF_Precision_HUNDREDTHS;
        const auto error=PF_ADD_PARAM(data,-1,&def);if(error)return error;}
    return PF_Err_NONE;
}

core::Result<ModelAuthorCapture> capture_model_author_controls(PF_InData* data,
    std::span<PF_ParamDef* const> params,const core::Cancellation& cancellation) noexcept {
    using R=core::Result<ModelAuthorCapture>;
    if(!data || params.size()<=layout::last)return invalid<ModelAuthorCapture>("incomplete Model author controls");
    if(cancellation.is_cancelled())return R::failure(core::ErrorCode::cancelled,"Model author capture cancelled");
    if(!right_type(params,layout::source,PF_Param_POPUP) || !right_type(params,layout::mesh,PF_Param_ARBITRARY_DATA) ||
        !right_type(params,layout::revision,PF_Param_SLIDER))return invalid<ModelAuthorCapture>("invalid Model source/mesh/revision control type");
    const auto native_source=params[layout::source]->u.pd.value;
    const auto revision=params[layout::revision]->u.sd.value;
    if((native_source!=1 && native_source!=2) || revision<0)return invalid<ModelAuthorCapture>("invalid Model source or revision");
    ModelAuthorCapture captured;captured.source=static_cast<std::uint32_t>(native_source-1);
    captured.revision=static_cast<std::uint32_t>(revision);
    double* vectors[]={&captured.pose.origin.x,&captured.pose.origin.y,&captured.pose.origin.z,
        &captured.pose.rotation_degrees.x,&captured.pose.rotation_degrees.y,&captured.pose.rotation_degrees.z,
        &captured.pose.scale_percent.x,&captured.pose.scale_percent.y,&captured.pose.scale_percent.z};
    for(int field=0;field<9;++field){const int index=layout::origin+field;const bool angle=field>=3 && field<6;
        if(!right_type(params,index,angle?PF_Param_ANGLE:PF_Param_FLOAT_SLIDER))return invalid<ModelAuthorCapture>("invalid Model pose control type");
        *vectors[field]=angle?double(params[index]->u.ad.value)/65536.0:params[index]->u.fs_d.value;}
    bool* booleans[]={&captured.pose.flip_x,&captured.pose.flip_y,&captured.pose.flip_z,&captured.pose.center,&captured.pose.normalize};
    for(int b=0;b<5;++b){const int index=layout::flip_x+b;
        if(!right_type(params,index,PF_Param_CHECKBOX))return invalid<ModelAuthorCapture>("invalid Model boolean control type");
        const auto value=params[index]->u.bd.value;
        if(value!=0 && value!=1)return invalid<ModelAuthorCapture>("invalid Model boolean control value");
        *booleans[b]=value!=0;}
    // Cube/placeholder need no stored OBJ resource. A parked old mesh may be
    // corrupt without making a selected default cube depend on it.
    auto geometry=captured.source==1 && captured.revision!=0 ?
        read_model_geometry_parameter(data,params[layout::mesh]->u.arb_d.value,cancellation):core::make_unit_cube();
    if(!geometry.has_value())return R::failure(geometry.error());
    captured.geometry=geometry.take_value();
    const auto matrix=core::model_local_matrix(captured.pose,captured.geometry.bounds);
    if(!matrix.has_value())return R::failure(matrix.error());
    if(cancellation.is_cancelled())return R::failure(core::ErrorCode::cancelled,"Model author capture cancelled");
    return R::success(std::move(captured));
}

core::Result<core::GraphNode> model_author_graph_node(core::NodeId id,const ModelAuthorCapture& captured,
    const core::Cancellation& cancellation) noexcept {
    using R=core::Result<core::GraphNode>;using namespace core::graph_keys;
    if(id.value.is_zero() || captured.source>1 || captured.revision>2147483647u)
        return invalid<core::GraphNode>("invalid Model author identity/source/revision");
    if(cancellation.is_cancelled())return R::failure(core::ErrorCode::cancelled,"Model graph author cancelled");
    const bool imported=captured.source==1 && captured.revision!=0;
    const auto bounds=imported?core::validate_model_geometry(captured.geometry,cancellation):
        core::Result<core::ModelBounds>::success({{-.5,-.5,-.5},{.5,.5,.5}});
    if(!bounds.has_value())return R::failure(bounds.error());
    const auto matrix=core::model_local_matrix(captured.pose,bounds.value());
    if(!matrix.has_value())return R::failure(matrix.error());
    try {
        core::OpaqueBytes resource(16);
        if(imported)for(unsigned b=0;b<16;++b)resource[b]=static_cast<std::byte>(id.value.bytes[b]);
        core::OpaqueBytes encoded_bounds;encoded_bounds.reserve(48);
        for(double value:{bounds.value().minimum.x,bounds.value().minimum.y,bounds.value().minimum.z,
            bounds.value().maximum.x,bounds.value().maximum.y,bounds.value().maximum.z})append_double(encoded_bounds,value);
        return R::success({id,kModelNode,1,{{kModelResource,std::move(resource)},
            {kModelRevision,imported?captured.revision:std::uint32_t{0}},
            {kModelOrigin,captured.pose.origin},{kModelRotation,captured.pose.rotation_degrees},{kModelScale,captured.pose.scale_percent},
            {kModelFlipX,std::uint32_t(captured.pose.flip_x)},{kModelFlipY,std::uint32_t(captured.pose.flip_y)},
            {kModelFlipZ,std::uint32_t(captured.pose.flip_z)},{kModelCenter,std::uint32_t(captured.pose.center)},
            {kModelNormalize,std::uint32_t(captured.pose.normalize)},{kModelBounds,std::move(encoded_bounds)},
            {kModelSource,captured.source}}});
    } catch(const std::bad_alloc&) {return R::failure(core::ErrorCode::allocation_failed,"Model graph author allocation failed");}
}

PF_Err prepare_model_obj_parameter(PF_InData* data,std::string_view text,PF_ArbitraryH* output,
    const core::Cancellation& cancellation,core::ModelBounds* bounds) noexcept {
    if(!output)return PF_Err_BAD_CALLBACK_PARAM;
    *output=nullptr;
    auto parsed=core::parse_model_obj(text,cancellation);
    if(!parsed.has_value())return host_error(parsed.error());
    const auto error=create_model_geometry_parameter(data,parsed.value(),output,cancellation);
    if(!error&&bounds)*bounds=parsed.value().bounds;return error;
}
}
