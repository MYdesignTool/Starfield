// Production native delete executor; reuse the existing public fake parade.
// No AE, scripts, actual focus, browser process or diagnostic journal.
#include "fixtures/effect_parade.hpp"
#include "NativeGraphEdit.hpp"
#include "GraphCommitRequest.hpp"
#include "GraphBindingRefreshRequest.hpp"
#include "NativeClearRequest.hpp"
#include "UiExclusionHost.hpp"
namespace {
AEGP_UtilitySuite6 edit_utility{};AEGP_LayerSuite9 edit_layers{};
decltype(SPBasicSuite::AcquireSuite) original_acquire{};
unsigned undo_starts{},undo_ends{},commit_calls{},refresh_calls{};
bool reject_commit{},missing_ack{},fail_refresh{},commit_error_after_publish{};
bool fail_postpublish_read{};
AEGP_ItemSuite9 edit_items{};AEGP_CompSuite11 edit_comps{};AEGP_CollectionSuite2 edit_collections{};AEGP_ProjSuite6 edit_projects{};
std::vector<AEGP_CollectionItemV2> selected_items;
unsigned collection_refs{};
AEGP_CommandSuite1 edit_commands{};
unsigned clear_calls{},clear_captures{},clear_publications{};
int clear_mode{};bool reject_capture{};A_Boolean* outer_clear_handled{};
std::vector<AuthorNodeId> requested;
AuthorNodeId author_id(const Ptr& p){AuthorNodeId id{};const auto raw=uuid(*p);for(std::size_t i=0;i<8;++i)id[i]=(unsigned(raw[i*2])<<8)|raw[i*2+1];return id;}
void setup(unsigned count=4){reset();parade.clear();parade.push_back(make(-1,0));
    for(unsigned n=1;n<=count;++n)parade.push_back(make(3,n));
    auto& main=*parade[0];main.numbers[kGraphRevisionId]=7;main.numbers[kGraphChecksumHighId]=1;main.numbers[kGraphChecksumLowId]=2;
    main.numbers[kGraphEditCommitId]=100;main.numbers[kGraphEditReceiptId]=100;main.numbers[kNodeEffectsReadyId]=1;main.numbers[kControlSourceId]=kNodeControlSource;
    for(std::size_t n=1;n<parade.size();++n){auto& p=*parade[n];const auto k=static_cast<native_nodes::Kind>(p.kind);
        p.numbers[native_nodes::connection_count_index(k)]=n+1<parade.size()?1:0;
        for(A_long i=0;i<64;++i)p.numbers[native_nodes::connection_first_index(k)+i]=0;
        if(n+1<parade.size()){const auto target=author_id(parade[n+1]);for(A_long i=0;i<8;++i)p.numbers[native_nodes::connection_first_index(k)+i]=target[i];}}
    CHECK(collection_refs==0);selected_items.clear();
    undo_starts=undo_ends=commit_calls=refresh_calls=0;reject_commit=missing_ack=fail_refresh=commit_error_after_publish=fail_postpublish_read=false;requested.clear();
    clear_calls=clear_captures=clear_publications=0;clear_mode=0;reject_capture=false;outer_clear_handled=nullptr;
}
NativeDeleteRequest request(std::initializer_list<unsigned> selected){NativeDeleteRequest r;r.revision=7;r.checksum=65538;
    for(auto i:selected)r.nodes.push_back(author_id(parade[i]));requested=r.nodes;return r;}
NativeDeleteResult run(const NativeDeleteRequest& r){HostUiExclusion exclusion;CHECK(exclusion);return delete_native_nodes(&basic,7,layer(),r);}
void init_edit(){initialize();original_acquire=basic.AcquireSuite;
    basic.AcquireSuite=[](const char* name,int32 version,const void** out)->A_Err{
        if(missing_suite==name){*out=nullptr;return injected;}
        if(!std::strcmp(name,kAEGPUtilitySuite)){CHECK(version==kAEGPUtilitySuiteVersion6);*out=&edit_utility;++suites;return 0;}
        if(!std::strcmp(name,kAEGPLayerSuite)){CHECK(version==kAEGPLayerSuiteVersion9);*out=&edit_layers;++suites;return 0;}
        if(!std::strcmp(name,kAEGPItemSuite)){*out=&edit_items;++suites;return 0;}
        if(!std::strcmp(name,kAEGPCompSuite)){*out=&edit_comps;++suites;return 0;}
        if(!std::strcmp(name,kAEGPCollectionSuite)){*out=&edit_collections;++suites;return 0;}
        if(!std::strcmp(name,kAEGPProjSuite)){*out=&edit_projects;++suites;return 0;}
        if(!std::strcmp(name,kAEGPCommandSuite)){*out=&edit_commands;++suites;return 0;}
        return original_acquire(name,version,out);};
    edit_utility.AEGP_StartUndoGroup=[](const A_char*)->A_Err{CHECK(SFLD_UiExclusionActiveV1());CHECK(SFLD_GraphMutationEpochV1()&1);++undo_starts;return fault("undo-start")==1?injected:0;};
    edit_utility.AEGP_EndUndoGroup=[]()->A_Err{CHECK(SFLD_GraphMutationEpochV1()&1);++undo_ends;return fault("undo-end")==1?injected:0;};
    edit_layers.AEGP_GetLayerCurrentTime=[](AEGP_LayerH h,AEGP_LTimeMode mode,A_Time* t)->A_Err{CHECK(h==layer() && mode==AEGP_LTimeMode_LayerTime);*t={12,24};return 0;};
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* out)->A_Err{if(fault("match")==1)return injected;
        const char* name{};switch(static_cast<int>(key)-3){case -2:name="third.party.blur";break;case -1:name="org.starfieldfx.particle";break;
        case 0:name="org.starfieldfx.node.emitter";break;case 1:name="org.starfieldfx.node.particle";break;case 3:name="org.starfieldfx.node.force";break;
        case 4:name="org.starfieldfx.node.transform";break;case 5:name="org.starfieldfx.node.model";break;case 6:name="org.starfieldfx.node.material";break;
        case 7:name="org.starfieldfx.node.motion";break;case 8:name="org.starfieldfx.node.turbulence";break;default:CHECK(false);return injected;}
        std::strcpy(out,name);return 0;};
    effects.AEGP_EffectCallGeneric=[](AEGP_PluginID p,AEGP_EffectRefH fx,const A_Time* t,PF_Cmd command,void* data)->A_Err{
        CHECK(p==7 && t->value==12 && t->scale==24 && command==PF_Cmd_COMPLETELY_GENERAL);
        if(*static_cast<std::uint32_t*>(data)!=0x53464331)CHECK(SFLD_GraphMutationEpochV1()&1);
        CHECK(stream_refs==0 && values==0);auto renderer=effect(fx);CHECK(renderer->kind==-1);
        if(*static_cast<std::uint32_t*>(data)==0x53464331){++clear_captures;auto& r=*static_cast<NativeClearRequest*>(data);
            CHECK(renderer->numbers[kGraphSyncGuardId]==0 && collection_refs==0);r.acknowledged=1;r.status=reject_capture?injected:0;
            r.snapshot->captured=!reject_capture;r.snapshot->time=*t;return r.status;}
        if(*static_cast<std::uint32_t*>(data)==0x53464332){++clear_publications;auto& r=*static_cast<NativeClearRequest*>(data);
            CHECK(r.snapshot->captured && r.snapshot->removed_count>0 && !native_clear_dispatch_active());
            requested.clear();for(std::uint32_t i=0;i<r.snapshot->removed_count;++i){AuthorNodeId id{};
                for(std::size_t n=0;n<8;++n)id[n]=static_cast<std::uint16_t>((unsigned(r.snapshot->removed[i].value.bytes[n*2])<<8)|r.snapshot->removed[i].value.bytes[n*2+1]);
                requested.push_back(id);for(const auto& e:parade)if(e->kind>=0)CHECK(author_id(e)!=id);}
            if(missing_ack)return injected;r.acknowledged=1;r.status=reject_commit?injected:0;if(reject_commit)return injected;
            ++renderer->numbers[kGraphRevisionId];++renderer->numbers[kGraphChecksumLowId];renderer->numbers[kGraphEditReceiptId]=r.nonce;
            return commit_error_after_publish?injected:0;}
        if(*static_cast<std::uint32_t*>(data)==0x53464731){++commit_calls;
            for(const auto& id:requested){unsigned found{};for(const auto& n:parade)if(n->kind>=0 && author_id(n)==id){CHECK(n->numbers[guard(n->kind)]==3);++found;}CHECK(found==1);}
            for(const auto& n:parade)if(n->kind>=0 && n->numbers[guard(n->kind)]==0){const auto k=static_cast<native_nodes::Kind>(n->kind);
                for(unsigned edge=0;edge<n->numbers[native_nodes::connection_count_index(k)];++edge){AuthorNodeId dest{};for(A_long i=0;i<8;++i)dest[i]=static_cast<std::uint16_t>(n->numbers[native_nodes::connection_first_index(k)+edge*16+i]);
                    CHECK(std::find(requested.begin(),requested.end(),dest)==requested.end());}}
            auto& r=*static_cast<GraphCommitRequest*>(data);if(missing_ack)return injected;r.acknowledged=1;r.status=reject_commit?injected:0;if(reject_commit)return injected;
            renderer->numbers[kGraphRevisionId]++;renderer->numbers[kGraphChecksumLowId]++;
            if(fail_postpublish_read)inject("scalar-read",1);
            renderer->numbers[kGraphEditReceiptId]=r.nonce;return commit_error_after_publish?injected:0;
        }
        CHECK(*static_cast<std::uint32_t*>(data)==0x53465231);++refresh_calls;auto& r=*static_cast<GraphBindingRefreshRequest*>(data);r.acknowledged=1;r.status=fail_refresh?injected:0;
        for(const auto& id:requested)for(const auto& n:parade)if(n->kind>=0)CHECK(author_id(n)!=id);
        return r.status;
    };
    edit_projects.AEGP_GetProjectByIndex=[](A_long index,AEGP_ProjectH* out)->A_Err{CHECK(index==0);*out=reinterpret_cast<AEGP_ProjectH>(2);return 0;};
    edit_projects.AEGP_GetProjectPath=[](AEGP_ProjectH,AEGP_MemHandle* out)->A_Err{auto* m=new Memory;m->data={0};++handles;*out=reinterpret_cast<AEGP_MemHandle>(m);return 0;};
    edit_items.AEGP_GetActiveItem=[](AEGP_ItemH* out)->A_Err{*out=reinterpret_cast<AEGP_ItemH>(3);return 0;};
    edit_items.AEGP_GetFirstProjItem=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=reinterpret_cast<AEGP_ItemH>(3);return 0;};
    edit_items.AEGP_GetNextProjItem=[](AEGP_ProjectH,AEGP_ItemH,AEGP_ItemH* out)->A_Err{*out=nullptr;return 0;};
    edit_items.AEGP_GetItemType=[](AEGP_ItemH,AEGP_ItemType* out)->A_Err{*out=AEGP_ItemType_COMP;return 0;};
    edit_items.AEGP_GetItemID=[](AEGP_ItemH,A_long* out)->A_Err{*out=88;return 0;};
    edit_comps.AEGP_GetCompFromItem=[](AEGP_ItemH,AEGP_CompH* out)->A_Err{*out=reinterpret_cast<AEGP_CompH>(4);return 0;};
    edit_layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err{*out=id==111?layer():nullptr;return 0;};
    edit_layers.AEGP_GetLayerID=[](AEGP_LayerH,A_long* out)->A_Err{*out=111;return 0;};
    edit_layers.AEGP_GetCompNumLayers=[](AEGP_CompH,A_long* out)->A_Err{*out=1;return 0;};
    edit_layers.AEGP_GetCompLayerByIndex=[](AEGP_CompH,A_long index,AEGP_LayerH* out)->A_Err{CHECK(index==0);*out=layer();return 0;};
    edit_comps.AEGP_GetNewCollectionFromCompSelection=[](AEGP_PluginID,AEGP_CompH,AEGP_Collection2H* out)->A_Err{++collection_refs;*out=reinterpret_cast<AEGP_Collection2H>(5);return 0;};
    edit_collections.AEGP_GetCollectionNumItems=[](AEGP_Collection2H,A_u_long* out)->A_Err{*out=static_cast<A_u_long>(selected_items.size());return 0;};
    edit_collections.AEGP_GetCollectionItemByIndex=[](AEGP_Collection2H,A_u_long i,AEGP_CollectionItemV2* out)->A_Err{*out=selected_items.at(i);return 0;};
    edit_collections.AEGP_DisposeCollection=[](AEGP_Collection2H)->A_Err{CHECK(collection_refs==1);--collection_refs;
        for(auto& s:selected_items)if(s.stream_refH){streams.AEGP_DisposeStream(s.stream_refH);s.stream_refH=nullptr;}return 0;};
    dynamic.AEGP_GetMatchName=[](AEGP_StreamRefH h,A_char* out)->A_Err{auto& s=stream(h);
        if(s.type==-2)std::strcpy(out,"ADBE Effect Parade");else if(s.type==-1){CHECK(s.effect);CHECK(effects.AEGP_GetEffectMatchName(static_cast<AEGP_InstalledEffectKey>(s.effect->kind+3),out)==0);}
        else std::strcpy(out,"ADBE Layer");return 0;};
    dynamic.AEGP_GetStreamGroupingType=[](AEGP_StreamRefH h,AEGP_StreamGroupingType* out)->A_Err{*out=stream(h).type==-1?AEGP_StreamGroupingType_NAMED_GROUP:AEGP_StreamGroupingType_LEAF;return 0;};
    dynamic.AEGP_GetNewParentStreamRef=[](AEGP_PluginID,AEGP_StreamRefH h,AEGP_StreamRefH* out)->A_Err{auto& s=stream(h);
        *out=s.type==0?ref(-1,s.effect):ref(s.type==-1?-2:-3);return 0;};
    dynamic.AEGP_GetStreamIndexInParent=[](AEGP_StreamRefH h,A_long* out)->A_Err{auto& s=stream(h);const auto found=std::find(parade.begin(),parade.end(),s.effect);
        CHECK(found!=parade.end());*out=static_cast<A_long>(found-parade.begin());return 0;};
    streams.AEGP_GetUniqueStreamID=[](AEGP_StreamRefH h,std::int32_t* out)->A_Err{CHECK(stream(h).type==-3);*out=1;return 0;};
    edit_commands.AEGP_DoCommand=[](AEGP_Command command)->A_Err{
        CHECK(command==21 && native_clear_dispatch_active() && SFLD_UiExclusionActiveV1() && (SFLD_GraphMutationEpochV1()&1));
        CHECK(outer_clear_handled && *outer_clear_handled && !effect_refs && !stream_refs && !values && !collection_refs && !handles && !locks);
        CHECK(parade[0]->numbers[kGraphSyncGuardId]==1);++clear_calls;
        if(clear_mode==1)return 0; // AE chose another target; residual ECW set stays alive.
        if(clear_mode==2){parade.erase(parade.begin()+1);return 0;} // Unselected source loss must not be certified.
        bool removed{};std::erase_if(parade,[&](const auto& e){if(e->kind<0)return false;
            const bool selected=std::find(requested.begin(),requested.end(),author_id(e))!=requested.end();
            if(selected && (clear_mode!=3 || !removed)){removed=true;return true;}return false;});
        if(clear_mode==4)missing_suite=kAEGPProjSuite;
        if(clear_mode==5)++parade[0]->numbers[kGraphRevisionId];
        return clear_mode==6?injected:0;
    };
}
void application_clear(){
    const auto select_effects=[](std::initializer_list<unsigned> indices){for(const auto index:indices){AEGP_CollectionItemV2 item{};
        item.type=AEGP_CollectionItemType_EFFECT;item.u.effect.layerH=layer();item.u.effect.index=static_cast<A_long>(index);selected_items.push_back(item);}};
    const auto clear=[](){A_Boolean handled=FALSE;outer_clear_handled=&handled;NativeDeleteResult r;
        CHECK(delete_selected_native_nodes(&basic,7,r,true,21,&handled));CHECK(handled && !native_clear_dispatch_active());outer_clear_handled=nullptr;return r;};
    for(const auto kind:{0,1,3,4,5,6,7,8}){setup();const auto id=uuid(*parade[2]);parade[2]=make(kind,2);uuid(*parade[2],id);
        const auto k=static_cast<native_nodes::Kind>(kind);parade[2]->numbers[native_nodes::connection_count_index(k)]=0;
        for(A_long i=0;i<64;++i)parade[2]->numbers[native_nodes::connection_first_index(k)+i]=0;
        request({2});select_effects({2});const auto r=clear();CHECK(r.committed && !r.guarded && !r.error && !r.cleanup_error && !r.undo_error && r.revision==8);
        CHECK(clear_calls==1 && clear_captures==1 && clear_publications==1 && refresh_calls==1 && undo_starts==1 && undo_ends==1);
        CHECK(calls["delete"]==0 && calls["duplicate"]==0);no_owned();}
    setup();request({2});select_effects({2});clear_mode=1;const auto no_change=snapshot();auto r=clear();
    CHECK(r.unchanged && !r.committed && !r.guarded && !r.error && clear_calls==1 && !clear_publications);
    CHECK(native_delete_reply(r).find(L"\"ok\":true")!=std::wstring::npos);equals(no_change);no_owned();
    setup();request({2,3});select_effects({2,3});clear_mode=3;r=clear();CHECK(r.committed && parade.size()==4 && clear_calls==1);no_owned();
    for(unsigned shape=0;shape<3;++shape){setup();request({2});AEGP_CollectionItemV2 item{};
        if(shape==0){item.type=AEGP_CollectionItemType_STREAM;item.u.stream.type=AEGP_StreamCollectionItemType_EFFECT;
            item.u.stream.u.effect_stream.effect={layer(),2};item.u.stream.u.effect_stream.param_index=1;}
        else if(shape==1){item.type=AEGP_CollectionItemType_KEYFRAME;item.u.keyframe.stream_coll.type=AEGP_StreamCollectionItemType_EFFECT;
            item.u.keyframe.stream_coll.u.effect_stream.effect={layer(),2};item.u.keyframe.stream_coll.u.effect_stream.param_index=1;}
        else {item.type=AEGP_CollectionItemType_STREAMREF;item.stream_refH=ref(0,parade[2],1);}
        selected_items.push_back(item);clear_mode=1;r=clear();
        CHECK(r.unchanged && !r.committed && !r.guarded && clear_calls==1 && !clear_publications && parade.size()==5);no_owned();}
    for(const int mode:{2,4,5}){setup();request({2});select_effects({2});clear_mode=mode;r=clear();
        CHECK(r.error && r.guarded && !r.committed && !clear_publications && undo_ends==1 && clear_calls==1);no_owned();}
    setup();request({2});select_effects({2});reject_capture=true;r=clear();CHECK(r.error && !r.guarded && !clear_calls && !undo_starts);no_owned();
    setup();request({2});select_effects({2});clear_mode=6;r=clear();CHECK(r.committed && !r.guarded && r.cleanup_error==injected);no_owned();
    setup();request({2});select_effects({2});missing_ack=true;r=clear();CHECK(!r.committed && r.outcome_unknown && r.guarded);no_owned();
    setup();request({2});select_effects({2});commit_error_after_publish=true;r=clear();CHECK(r.committed && r.revision==8 && r.guarded && r.cleanup_error==injected);no_owned();
    setup();request({2});select_effects({2});inject("undo-end",1);r=clear();CHECK(r.committed && !r.guarded && r.undo_error==injected);no_owned();
}
void success(){for(const auto type:{0,1,3,4,5,6,7,8}){setup();auto changed=parade[2];const auto old_id=uuid(*changed);
        changed->kind=type;changed->numbers.clear();uuid(*changed,old_id);changed->numbers[guard(type)]=0;
        const auto k=static_cast<native_nodes::Kind>(type);changed->numbers[native_nodes::connection_count_index(k)]=0;
        for(A_long i=0;i<64;++i)changed->numbers[native_nodes::connection_first_index(k)+i]=0;
        const auto r=run(request({2}));CHECK(r.committed && !r.error && !r.cleanup_error && !r.guarded && r.revision==8);
        CHECK(parade.size()==4 && undo_starts==1 && undo_ends==1 && commit_calls==1 && refresh_calls==1);
        CHECK(calls["duplicate"]==0 && calls["delete"]==1);no_owned();}
    setup();const auto r=run(request({1,3,4}));CHECK(r.committed && parade.size()==2 && calls["delete"]==3);no_owned();
    setup();std::swap(parade[0],parade[4]);const auto moved=run(request({1,2}));CHECK(moved.committed && !moved.guarded && parade.size()==3);no_owned();
}
void failures(){for(unsigned at=1;at<=25;++at){setup();const auto req=request({2});const auto original=snapshot();inject("scalar-write",at);
        const auto r=run(req);if(!r.committed){CHECK(r.error);if(!r.guarded)equals(original);}CHECK(undo_starts==undo_ends);no_owned();}
    setup();auto req=request({2});auto original=snapshot();req.revision=6;CHECK(run(req).error && undo_starts==0);equals(original);no_owned();
    setup();req=request({2});original=snapshot();inject("undo-start");CHECK(run(req).error && undo_ends==0);equals(original);no_owned();
    setup();req=request({2});reject_commit=true;auto r=run(req);CHECK(!r.committed && r.error && r.guarded && parade.size()==5 && calls["delete"]==0);no_owned();
    setup();req=request({2});missing_ack=true;r=run(req);CHECK(!r.committed && r.guarded && calls["delete"]==0);no_owned();
    setup();req=request({2});inject("delete");r=run(req);CHECK(r.committed && r.cleanup_error && r.guarded && parade.size()==5);no_owned();
    setup();req=request({2});fail_refresh=true;r=run(req);CHECK(r.committed && r.cleanup_error && r.guarded && parade.size()==4);no_owned();
    setup();req=request({2});commit_error_after_publish=true;r=run(req);CHECK(r.committed && r.cleanup_error && r.guarded && calls["delete"]==0);no_owned();
    setup();req=request({2});inject("undo-end");r=run(req);CHECK(r.committed && r.undo_error && !r.guarded);no_owned();
    setup();req=request({2});fail_postpublish_read=true;r=run(req);CHECK(r.committed && r.cleanup_error && r.guarded && parade[0]->numbers[kGraphEditReceiptId]==101);
    CHECK(parade[2]->numbers[guard(3)]==3 && parade[1]->numbers[native_nodes::connection_count_index(native_nodes::Kind::force)]==1);
    for(A_long i=0;i<8;++i)CHECK(parade[1]->numbers[native_nodes::connection_first_index(native_nodes::Kind::force)+i]==author_id(parade[3])[i]);no_owned();
    setup();req=request({2});req.nodes.push_back(req.nodes[0]);CHECK(run(req).error && undo_starts==0);no_owned();
    setup();req=request({2});parade[1]->numbers[guard(3)]=1;CHECK(run(req).error && undo_starts==0);no_owned();
}
void selection(){for(bool stream_selection:{false,true}){setup();(void)request({2});AEGP_CollectionItemV2 selected{};
        if(stream_selection){selected.type=AEGP_CollectionItemType_STREAMREF;selected.stream_refH=ref(-1,parade[2]);}
        else {selected.type=AEGP_CollectionItemType_EFFECT;selected.u.effect={layer(),2};}
        selected_items.push_back(selected);NativeDeleteResult r;CHECK(delete_selected_native_nodes(&basic,7,r));
        CHECK(r.committed && !r.guarded && r.comp==88 && r.layer==111 && !collection_refs);no_owned();}
    setup();AEGP_CollectionItemV2 selected{};selected.type=AEGP_CollectionItemType_KEYFRAME;selected_items.push_back(selected);NativeDeleteResult r;
    CHECK(!delete_selected_native_nodes(&basic,7,r) && !undo_starts && !collection_refs);no_owned();
    setup();selected={};selected.type=AEGP_CollectionItemType_STREAMREF;selected.stream_refH=ref(0,parade[2],1);selected_items.push_back(selected);
    CHECK(!delete_selected_native_nodes(&basic,7,r) && !undo_starts && !collection_refs);no_owned();
    setup();{HostUiExclusion outer;CHECK(outer);CHECK(delete_selected_native_nodes(&basic,7,r));CHECK(r.error && !collection_refs && !undo_starts);}no_owned();
    for(bool fail_item:{false,true}){setup();selected={};selected.type=AEGP_CollectionItemType_EFFECT;selected.u.effect={layer(),2};selected_items.push_back(selected);
        const auto item_getter=edit_items.AEGP_GetItemID;const auto layer_getter=edit_layers.AEGP_GetLayerID;
        if(fail_item)edit_items.AEGP_GetItemID=[](AEGP_ItemH,A_long*)->A_Err{return injected;};
        else edit_layers.AEGP_GetLayerID=[](AEGP_LayerH,A_long*)->A_Err{return injected;};
        const auto original=snapshot();CHECK(delete_selected_native_nodes(&basic,7,r));CHECK(r.error==injected && !undo_starts && !commit_calls);
        equals(original);no_owned();edit_items.AEGP_GetItemID=item_getter;edit_layers.AEGP_GetLayerID=layer_getter;}
    setup();auto req=request({2});std::wstring delta=L"p0-c88-l111|7|00010002||";constexpr wchar_t chars[]=L"0123456789abcdef";
    for(auto word:req.nodes[0])for(int shift=12;shift>=0;shift-=4)delta+=chars[(word>>shift)&15];
    {HostUiExclusion outer;CHECK(outer);const auto reply=execute_native_delete(&basic,7,delta);CHECK(reply.find(L"\"ok\":true")!=reply.npos);}no_owned();
}
void parser(){NativeDeleteRequest req;
    CHECK(parse_native_delete(L"p0-c88-l111|7|00010002||0102030405060708090a0b0c0d0e0f10",req));CHECK(req.comp==88 && req.layer==111 && req.revision==7 && req.checksum==65538);
    for(const auto* s:{L"",L"p1-c88-l111|7|00010002||0102030405060708090a0b0c0d0e0f10",L"p0-c88-l111|0|00010002||0102030405060708090a0b0c0d0e0f10",
        L"p0-c88-l111|7|00010002||000000000000000000000000000000ff",L"p0-c88-l111|7|00010002||0102030405060708090a0b0c0d0e0f10,"})CHECK(!parse_native_delete(s,req));
}
NativeConnection saved_connection(unsigned source,unsigned which=0){NativeConnection c;const auto& n=*parade[source];const auto first=native_nodes::connection_first_index(static_cast<native_nodes::Kind>(n.kind))+16*which;
    for(A_long i=0;i<8;++i){c.target[i]=static_cast<std::uint16_t>(n.numbers.at(first+i));c.edge[i]=static_cast<std::uint16_t>(n.numbers.at(first+8+i));}return c;}
void edge_id_fixture(){for(unsigned n=1;n+1<parade.size();++n)parade[n]->numbers[native_nodes::connection_first_index(static_cast<native_nodes::Kind>(parade[n]->kind))+15]=200+n;}
NativeDeleteRequest connection_patch(bool growth){auto req=request({});req.patch=true;edge_id_fixture();const auto old=saved_connection(1);auto one=old;one.target=author_id(parade[4]);
    NativeConnectionRow row{author_id(parade[1]),{old},{one}};
    if(growth){auto two=one;two.target=author_id(parade[3]);two.edge.back()=500;row.desired.push_back(two);const auto first=native_nodes::connection_first_index(native_nodes::Kind::force);for(A_long i=16;i<32;++i)parade[1]->numbers[first+i]=91+i;}
    req.rows.push_back(row);return req;}
void patches(){
    for(bool growth:{false,true}){setup();auto req=connection_patch(growth);const auto desired=req.rows[0].desired;auto r=run(req);
        CHECK(r.committed && !r.guarded && !r.cleanup_error && !r.error && calls["delete"]==0 && refresh_calls==0 && commit_calls==1);
        CHECK(saved_connection(1)==desired[0]);if(growth)CHECK(saved_connection(1,1)==desired[1]);no_owned();}
    setup();auto req=connection_patch(true);auto original=snapshot();req.rows[0].expected[0].target=author_id(parade[4]);auto r=run(req);CHECK(r.error && !undo_starts);equals(original);no_owned();
    setup();req=connection_patch(true);original=snapshot();req.rows.push_back(req.rows[0]);r=run(req);CHECK(r.error && !undo_starts);equals(original);no_owned();
    for(unsigned at=1;at<=75;++at){setup();req=connection_patch(true);original=snapshot();inject("scalar-write",at);r=run(req);
        CHECK(undo_starts==undo_ends);if(!r.committed && !r.rollback_error)for(std::size_t i=1;i<parade.size();++i)CHECK(*parade[i]==original[i]);no_owned();}
    setup();req=connection_patch(true);original=snapshot();reject_commit=true;r=run(req);CHECK(!r.committed && r.guarded && !r.rollback_error && !refresh_calls);
    for(std::size_t i=1;i<parade.size();++i)CHECK(*parade[i]==original[i]);no_owned();
    NativeDeleteRequest parsed;const std::wstring node=L"0102030405060708090a0b0c0d0e0f10",other=L"1102030405060708090a0b0c0d0e0f10",edge=L"2102030405060708090a0b0c0d0e0f10";
    CHECK(parse_native_patch(L"p0-c88-l111|7|00010002|||"+node+L":>"+other+edge,parsed));CHECK(parsed.patch && parsed.nodes.empty() && parsed.rows.size()==1 && parsed.rows[0].expected.empty() && parsed.rows[0].desired.size()==1);
    CHECK(!parse_native_patch(L"p0-c88-l111|7|00010002|||"+node+L":>"+other+edge+L";"+node+L":>",parsed));
    CHECK(!parse_native_patch(L"p0-c88-l111|7|00010002|||",parsed));
}
void clear_capacity(){setup(9);
    const auto rows=[](unsigned source,const std::vector<unsigned>& targets){auto& p=*parade[source];const auto k=static_cast<native_nodes::Kind>(p.kind);p.numbers[native_nodes::connection_count_index(k)]=static_cast<double>(targets.size());
        for(A_long i=0;i<64;++i)p.numbers[native_nodes::connection_first_index(k)+i]=0;
        for(std::size_t j=0;j<targets.size();++j){const auto id=author_id(parade[targets[j]]);for(A_long i=0;i<8;++i)p.numbers[native_nodes::connection_first_index(k)+static_cast<A_long>(j)*16+i]=id[i];p.numbers[native_nodes::connection_first_index(k)+static_cast<A_long>(j)*16+15]=200+static_cast<double>(j);}};
    rows(1,{2,7,8,9});rows(2,{3,4});for(unsigned i=3;i<parade.size();++i)rows(i,{});
    request({2});AEGP_CollectionItemV2 item{};item.type=AEGP_CollectionItemType_EFFECT;item.u.effect={layer(),2};selected_items.push_back(item);
    const auto before=snapshot();A_Boolean handled=FALSE;outer_clear_handled=&handled;NativeDeleteResult result;
    CHECK(delete_selected_native_nodes(&basic,7,result,true,21,&handled));CHECK(handled);outer_clear_handled=nullptr;
    CHECK(result.error && result.capacity_rejected && !result.clear_dispatched && !result.guarded && !undo_starts && !clear_calls);
    CHECK(native_delete_reply(result).find(L"connection_capacity")!=std::wstring::npos);
    equals(before);no_owned();
}
void counts(){for(unsigned n:{3u,8u,16u,32u,63u}){setup(n);const auto r=run(request({2}));CHECK(r.committed && !r.guarded);CHECK(calls["duplicate"]==0);
    std::cout<<"nodes="<<n<<" scalar-read="<<calls["scalar-read"]<<" effect-ref="<<calls["effect-ref"]<<" duplicates="<<calls["duplicate"]<<'\n';no_owned();}}
}
int main(){initialize_ui_exclusion();init_edit();parser();success();failures();selection();counts();application_clear();patches();clear_capacity();stop_ui_exclusion();
    std::cout<<checks<<" checks, 0 failures; production native editor, fake May2023 publisher (not AE binding/timing qualification)\n";return 0;}
