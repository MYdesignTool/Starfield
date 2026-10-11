#include "LightCapture.hpp"
#include "SPBasic.h"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace starfield::adapter;
namespace {
unsigned checks{},failures{},acquired{},released{},refs{},values{},calls{};
unsigned fail_call{},active_mask=15;A_long light_count=4;double falloff=2;bool cancelled{};
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};AEGP_StreamSuite6 streams{};AEGP_LightSuite2 lights{};
struct Ref {AEGP_LayerStream key;};
void check(bool condition,const char* name){++checks;if(!condition){++failures;std::printf("FAIL %s\n",name);}}
A_Err step(){return ++calls==fail_call?516:0;}
void identity(A_Matrix4& m){m={};for(unsigned i=0;i<4;++i)m.mat[i][i]=1;}
AEGP_LayerH handle(unsigned n){return reinterpret_cast<AEGP_LayerH>(static_cast<uintptr_t>(n));}
A_Err acquire(const char* name,int32,const void** out){auto e=step();if(e)return e;
 if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&pf;else if(!std::strcmp(name,kAEGPLayerSuite))*out=&layers;
 else if(!std::strcmp(name,kAEGPStreamSuite))*out=&streams;else if(!std::strcmp(name,kAEGPLightSuite))*out=&lights;else return 516;++acquired;return 0;}
A_Err release(const char*,int32){++released;return 0;}
}
int main(){
 SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
 pf.AEGP_ConvertEffectToCompTime=[](PF_ProgPtr,A_long value,A_u_long scale,A_Time* out)->A_Err{*out={value+static_cast<A_long>(scale),scale};return step();};
 pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* out)->A_Err{*out=handle(99);return step();};
 pf.AEGP_GetEffectCameraMatrix=[](PF_ProgPtr,const A_Time* t,A_Matrix4* m,A_FpLong* zoom,A_short* w,A_short* h)->A_Err{
  check(t->value==45&&t->scale==30,"comp time camera");identity(*m);m->mat[3][2]=-1000;*zoom=1000;*w=*h=100;return step();};
 layers.AEGP_GetLayerParentComp=[](AEGP_LayerH,AEGP_CompH* out)->A_Err{*out=reinterpret_cast<AEGP_CompH>(1);return step();};
 layers.AEGP_GetLayerToWorldXform=[](AEGP_LayerH layer,const A_Time*,A_Matrix4* out)->A_Err{identity(*out);out->mat[3][0]=layer==handle(99)?100:10;return step();};
 layers.AEGP_GetCompNumLayers=[](AEGP_CompH,A_long* out)->A_Err{*out=light_count;return step();};
 layers.AEGP_GetCompLayerByIndex=[](AEGP_CompH,A_long index,AEGP_LayerH* out)->A_Err{*out=handle(index+1);return step();};
 layers.AEGP_GetLayerObjectType=[](AEGP_LayerH,AEGP_ObjectType* out)->A_Err{*out=AEGP_ObjectType_LIGHT;return step();};
 layers.AEGP_IsVideoActive=[](AEGP_LayerH layer,AEGP_LTimeMode mode,const A_Time* t,A_Boolean* out)->A_Err{
  check(mode==AEGP_LTimeMode_CompTime&&t->value==45,"active comp time");const auto index=static_cast<unsigned>(reinterpret_cast<uintptr_t>(layer))-1;*out=index>=32||bool(active_mask&(1u<<index));return step();};
 lights.AEGP_GetLightType=[](AEGP_LayerH layer,AEGP_LightType* out)->A_Err{const AEGP_LightType types[]{AEGP_LightType_AMBIENT,AEGP_LightType_PARALLEL,AEGP_LightType_POINT,AEGP_LightType_SPOT};
  *out=types[(reinterpret_cast<uintptr_t>(layer)-1)%4];return step();};
 streams.AEGP_GetNewLayerStream=[](AEGP_PluginID,AEGP_LayerH,AEGP_LayerStream key,AEGP_StreamRefH* out)->A_Err{auto e=step();if(e)return e;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{key});++refs;return 0;};
 streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode mode,const A_Time* t,A_Boolean pre,AEGP_StreamValue2* out)->A_Err{
  check(mode==AEGP_LTimeMode_CompTime&&t->value==45&&!pre,"animated light sample time");auto e=step();if(e)return e;*out={};out->streamH=ref;++values;
  switch(reinterpret_cast<Ref*>(ref)->key){case AEGP_LayerStream_COLOR:out->val.color.redF=.2;out->val.color.greenF=.4;out->val.color.blueF=.8;break;
   case AEGP_LayerStream_INTENSITY:out->val.one_d=150;break;case AEGP_LayerStream_CONE_ANGLE:out->val.one_d=45;break;case AEGP_LayerStream_CONE_FEATHER:out->val.one_d=25;break;
   case AEGP_LayerStream_LIGHT_FALLOFF_TYPE:out->val.one_d=falloff;break;case AEGP_LayerStream_LIGHT_FALLOFF_START:out->val.one_d=50;break;case AEGP_LayerStream_LIGHT_FALLOFF_DISTANCE:out->val.one_d=100;break;default:return 516;}return 0;};
 streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{--values;return 0;};
 streams.AEGP_DisposeStream=[](AEGP_StreamRefH ref)->A_Err{delete reinterpret_cast<Ref*>(ref);--refs;return 0;};
 PF_InData data{};data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.pica_basicP=&basic;data.current_time=15;data.time_scale=30;
 data.inter.abort=[](PF_ProgPtr)->PF_Err{return cancelled?PF_Interrupt_CANCEL:PF_Err_NONE;};
 LightStaging result;check(capture_lights(nullptr,0,false,result)==0&&acquired==0,"unlit needs no AE suite");
 check(capture_lights(&data,1,true,result)==0&&result.lights.size()==4,"four light types captured");const auto total=calls;
 check(result.lights[0].type==0&&result.lights[1].type==1&&result.lights[2].type==2&&result.lights[3].type==3,"type mapping");
 check(result.lights[2].falloff==2&&result.lights[2].radius==50&&result.lights[3].cone_feather==.25,"falloff and spot normalization");
 check(result.layer_to_world[12]==100&&result.camera_enabled&&result.camera_position[2]==-1000&&result.lights[0].intensity==1.5,"world and intensity");
 SfCoreRenderRequest request{};apply_lights(request,result);check(request.light_count==4&&request.lights==result.lights.data()&&request.lighting_layer_to_world[12]==100,"borrowed numeric ABI tail");
 for(fail_call=1;fail_call<=total;++fail_call){calls=0;auto e=capture_lights(&data,1,true,result);if(fail_call!=10)check(e!=0,"injected suite failure propagates");check(refs==0&&values==0&&acquired==released,"failure disposes all ownership");}
 fail_call=0;calls=0;active_mask=1;check(capture_lights(&data,1,true,result)==0&&result.lights.size()==1,"AE active state honored");active_mask=15;
 for(double mode:{0.,1.,2.}){falloff=mode;check(capture_lights(&data,1,true,result)==0&&result.lights[2].falloff==mode,"SDK falloff enums");}
 falloff=3;check(capture_lights(&data,1,true,result)!=0,"invalid falloff rejected");falloff=2;
 cancelled=true;check(capture_lights(&data,1,true,result)==PF_Interrupt_CANCEL,"cancel propagates");cancelled=false;
 active_mask=~0u;light_count=65;check(capture_lights(&data,1,true,result)!=0,"light count bounded");
 check(refs==0&&values==0&&acquired==released,"final ownership balanced");std::printf("Light capture: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
