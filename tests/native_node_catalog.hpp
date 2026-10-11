#pragma once
#include <cstdio>
#include <cstdlib>
#include <vector>

// Test-only inventory from the real PF_Cmd_PARAMS_SETUP registration. It has
// no host calls or file writes and is enabled only for the catalog test run.
inline void print_native_node_catalog(const char* kind,const std::vector<PF_ParamDef>& controls) {
    if(!std::getenv("STARFIELD_TEST_NODE_CATALOG"))return;
    std::printf("NODE_CONTROL_CATALOG:{\"kind\":\"%s\",\"controls\":[",kind);
    unsigned index=0;
    for(const auto& control:controls) {
        if(index)std::printf(",");
        std::printf("{\"name\":\"");
        for(const char* p=control.name;*p;++p){if(*p=='\\' || *p=='\"')std::printf("\\");std::printf("%c",*p);}
        std::printf("\",\"disk\":%ld,\"index\":%u,\"type\":%d,\"value\":",control.uu.id,++index,static_cast<int>(control.param_type));
        switch(control.param_type) {
            case PF_Param_FLOAT_SLIDER:std::printf("%.17g",control.u.fs_d.value);break;
            case PF_Param_SLIDER:std::printf("%ld",control.u.sd.value);break;
            case PF_Param_POPUP:std::printf("%d,\"minimum\":1,\"maximum\":%d",control.u.pd.value,control.u.pd.num_choices);break;
            case PF_Param_CHECKBOX:std::printf("%d",static_cast<int>(control.u.bd.value));break;
            case PF_Param_LAYER:std::printf("0");break;
            case PF_Param_ANGLE:std::printf("%.17g",static_cast<double>(control.u.ad.value)/65536);break;
            case PF_Param_POINT:std::printf("[%.17g,%.17g]",static_cast<double>(control.u.td.x_value)/65536,static_cast<double>(control.u.td.y_value)/65536);break;
            case PF_Param_POINT_3D:std::printf("[%.17g,%.17g,%.17g]",control.u.point3d_d.x_value,control.u.point3d_d.y_value,control.u.point3d_d.z_value);break;
            case PF_Param_COLOR:std::printf("[%.17g,%.17g,%.17g,1]",control.u.cd.value.red/255.0,control.u.cd.value.green/255.0,control.u.cd.value.blue/255.0);break;
            default:std::printf("null");break;
        }
        std::printf("}");
    }
    std::printf("]}\n");
}
