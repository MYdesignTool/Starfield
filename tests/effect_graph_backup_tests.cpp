#include "fixtures/effect_parade.hpp"
namespace {
void mutate(){
    // Full author edits, deleted nodes and newly created nodes after backup.
    parade.erase(std::remove_if(parade.begin(),parade.end(),[](const Ptr& p){return p->kind>=0 && p->numbers.at(guard(p->kind))!=2;}),parade.end());
    for(const auto& p:parade)if(p->kind==-1 && p->numbers.at(guard(-1))!=2){p->name=u"Changed renderer";p->animation.clear();p->arbitrary.clear();}
    parade.push_back(make(5,61));parade.push_back(make(4,62));
}
void success_tests(){
    for(bool at_end:{false,true}){reset();duplicate_at_end=at_end;const auto saved=snapshot();const auto third_a=parade[0],third_b=parade[3];
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);no_refs();CHECK(parade.size()==14);CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);
            std::vector<UUID> ids;std::vector<std::u16string> backup_names;unsigned copies{};
            for(const auto& p:parade)if(p->kind>=-1 && p->numbers.at(guard(p->kind))==2){++copies;CHECK(!(p->flags&AEGP_EffectFlags_ACTIVE));
                auto observed=*p;observed.flags|=AEGP_EffectFlags_ACTIVE;observed.numbers[guard(p->kind)]=0;
                const auto found=std::find_if(saved.begin(),saved.end(),[&](const Effect& old){return old.kind==p->kind;});CHECK(found!=saved.end());
                if(p->kind>=0){const auto id=uuid(*p);CHECK(id!=uuid(*found));CHECK(std::find(ids.begin(),ids.end(),id)==ids.end());ids.push_back(id);uuid(observed,uuid(*found));}
                CHECK(p->name!=found->name);CHECK(std::find(backup_names.begin(),backup_names.end(),p->name)==backup_names.end());backup_names.push_back(p->name);
                observed.name=found->name;CHECK(observed==*found);p->name=u"Renamed backup";}
            CHECK(copies==6);mutate();releases.clear();CHECK(backup.restore()==0);no_refs();CHECK(backup.rollback_error()==0);equals(saved);
            CHECK(parade[0]==third_a && parade[3]==third_b);CHECK(releases.size()==6 && releases.back().first==-1);CHECK(backup.restore()!=0);}
        no_owned();
    }
    reset();const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);mutate();}equals(saved);no_owned();
    reset();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);parade[1]->arbitrary[99]={9,9,9};const auto original=parade[1];
        CHECK(backup.discard()==0);CHECK(parade.size()==8 && parade[1]==original);CHECK(parade[1]->arbitrary.contains(99));CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==0);no_refs();}
    no_owned();
    reset();UUID collision=operation;collision[0]^=0x80;collision.back()^=1;
    {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation,std::span<const UUID>(&collision,1))==0);
        for(const auto& p:parade)if(p->kind>=0 && p->numbers.at(guard(p->kind))==2)CHECK(uuid(*p)!=collision);CHECK(backup.discard()==0);}no_owned();
    reset();UUID edge{};edge[0]=0x80;edge.back()=0xfe;
    {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(edge)==0);for(const auto& p:parade)if(p->kind>=0 && p->numbers.at(guard(p->kind))==2){UUID output{};output.back()=0xff;CHECK(uuid(*p)!=output);CHECK(uuid(*p)!=UUID{});}CHECK(backup.discard()==0);}no_owned();
}
void preflight_tests(){
    for(unsigned removed=0;removed<=5;++removed){reset();const auto saved=snapshot();
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);
            unsigned erased{};
            for(auto it=parade.begin();it!=parade.end();){auto& p=*it;
                if(p->kind>=0&&p->numbers.at(guard(p->kind))!=transaction_backup_guard){
                    p->numbers[guard(p->kind)]=transaction_removal_guard;
                    if(erased++<removed){it=parade.erase(it);continue;}
                }++it;
            }
            CHECK(backup.restore()==0);equals(saved);no_refs();
        }no_owned();
    }
    for(const char* missing:{kAEGPEffectSuite,kAEGPStreamSuite,kAEGPDynamicStreamSuite,kAEGPMemorySuite}){reset();missing_suite=missing;const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);}equals(saved);no_owned();}
    for(unsigned case_id=0;case_id<10;++case_id){reset();
        switch(case_id){case 0:parade[2]->numbers[guard(5)]=1;break;case 1:parade[2]->numbers[guard(5)]=2;break;
            case 2:uuid(*parade[2],{});break;case 3:uuid(*parade[4],uuid(*parade[2]));break;case 4:parade.push_back(make(-1,9));break;
            case 5:parade.erase(parade.begin()+1);break;case 6:parade[2]->name=std::u16string(512,u'x');break;
            case 7:parade[2]->numbers[first(5)]=0.5;break;case 8:parade[2]->numbers[first(5)]=65536;break;case 9:wrong_type=true;break;}
        const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);CHECK(calls["scalar-write"]==0 && calls["duplicate"]==0);no_owned();}
    reset();const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare({})!=0);}equals(saved);no_owned();
    reset();for(unsigned id=0;id<60;++id)parade.push_back(make(5,100+id));const auto many=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);}equals(many);CHECK(calls["duplicate"]==0);no_owned();
    for(const char* op:{"count","effect-ref","key","match","flags-read","param-ref","param-type","scalar-read","root-ref","parade-ref","group-ref","name-read","mem-size","mem-lock","mem-unlock"}){
        reset();const auto old=snapshot();inject(op);{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);}equals(old);CHECK(calls["scalar-write"]==0);no_owned();}
}
void prepare_failure_tests(){
    reset();unsigned writes{};{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);writes=calls["scalar-write"];CHECK(backup.discard()==0);}no_owned();
    for(bool no_change:{false,true})for(unsigned at=1;at<=writes;++at){reset();const auto saved=snapshot();inject("scalar-write",at,no_change);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    for(bool no_change:{false,true})for(unsigned at=1;at<=6;++at){reset();const auto saved=snapshot();inject("flags-write",at,no_change);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    for(bool no_change:{false,true})for(unsigned at=1;at<=6;++at){reset();const auto saved=snapshot();inject("name-write",at,no_change);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    for(bool with_ref:{false,true})for(unsigned at=1;at<=6;++at){reset();const auto saved=snapshot();duplicate_error_with_ref=with_ref;inject("duplicate",at);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==injected);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    reset();duplicate_wrong_kind=true;const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();
    for(bool with_ref:{false,true})for(bool no_change:{false,true})for(bool once:{false,true}){
        reset();const auto original=parade[1];duplicate_error_with_ref=with_ref;inject("duplicate",2);
        cleanup_delete_error=!no_change;cleanup_delete_silent=no_change;cleanup_delete_once=once;
        const A_Err expected_cleanup=no_change?PF_Err_INTERNAL_STRUCT_DAMAGED:first_cleanup_error;
        {EffectGraphBackup backup(&basic,7,layer());
            CHECK(backup.prepare(operation)==injected);
            CHECK(backup.rollback_error()==expected_cleanup);
            CHECK(original->numbers.at(kGraphSyncGuardId)==1);
            if(with_ref && once)CHECK(std::none_of(parade.begin(),parade.end(),[](const Ptr& p){
                return p->kind>=-1 && p->numbers.at(guard(p->kind))==transaction_backup_guard;}));
            CHECK(cleanup_delete_calls>=1);no_refs();}
        CHECK(original->numbers.at(kGraphSyncGuardId)==1);no_owned();
    }
}
void restore_failure_tests(){
    for(const char* op:{"delete","reorder","name-write","flags-write","scalar-write"})for(bool no_change:{false,true}){
        reset();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);mutate();
            // Ensure reorder and flags writes change a value, so a silent failure is observable.
            std::reverse(parade.begin(),parade.end());for(const auto& p:parade)if(p->kind>=-1 && p->numbers.at(guard(p->kind))==2)p->name=u"Changed backup";
            inject(op,1,no_change);CHECK(backup.restore()!=0);CHECK(backup.rollback_error()!=0);no_refs();}no_owned();}
    for(bool no_change:{false,true}){reset();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);parade[1]->arbitrary[99]={99};inject("delete",1,no_change);
        CHECK(backup.discard()!=0);CHECK(parade[1]->arbitrary.contains(99));CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);
        CHECK(std::count_if(parade.begin(),parade.end(),[](const Ptr& p){return p->kind>=-1 && p->numbers.at(guard(p->kind))==2;})==1);no_refs();}no_owned();}
}
void final_parade_tests(){
    for(bool fails:{false,true}) {
        reset();unsigned final_calls{};std::pair<unsigned*,bool> context{&final_calls,fails};
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);
            const auto result=backup.discard([](void* ptr)->A_Err {
                auto& state=*static_cast<std::pair<unsigned*,bool>*>(ptr);++*state.first;no_refs();
                for(const auto& p:parade)if(p->kind>=-1)CHECK(p->numbers.at(guard(p->kind))!=transaction_backup_guard);
                CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);
                return state.second?injected:0;
            },&context);
            CHECK(result==(fails?injected:0));}
        CHECK(final_calls==1 && parade[1]->numbers.at(kGraphSyncGuardId)==(fails?1:0));no_owned();
    }
    for(bool fails:{false,true}) {
        reset();const auto saved=snapshot();unsigned final_calls{};
        std::pair<unsigned*,bool> context{&final_calls,fails};
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);mutate();
            const auto result=backup.restore([](void* ptr)->A_Err {
                auto& state=*static_cast<std::pair<unsigned*,bool>*>(ptr);++*state.first;no_refs();
                for(const auto& p:parade)if(p->kind>=-1)CHECK(p->numbers.at(guard(p->kind))!=transaction_backup_guard);
                CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);
                return state.second?injected:0;
            },&context);
            CHECK(result==(fails?injected:0));CHECK(backup.rollback_error()==result);}
        CHECK(final_calls==1 && parade[1]->numbers.at(kGraphSyncGuardId)==(fails?1:0));
        if(!fails)equals(saved);no_owned();
    }
}
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
struct Transaction {
    std::vector<std::uint8_t> bytes;
    std::vector<ModelAssetRestore> assets;
    std::vector<UUID> ids;
    unsigned starts{},ends{},depth{},prepares{},commits{},generics{},written{},finalizes{};
    unsigned cancel_phase{},generic_fail_at{};
    bool start_fail{},end_fail{},prepare_fail{},commit_fail{},prepare_throw{},commit_throw{},unacknowledged{},local_failure{},
        wrong_target{},duplicate_target{},busy_target{},silent_revision{},finalize_fail{};
    ModelGraphTransactionPlan plan() {ModelGraphTransactionPlan p;p.basic=&basic;p.plugin=7;p.layer=layer();p.time={12,24};p.id=operation;p.desired_ids=ids;p.assets=assets;p.context=this;
        p.prepare=[](void* context)->A_Err{auto& t=*static_cast<Transaction*>(context);++t.prepares;CHECK(t.depth==1);no_refs();
            // The actual gateway callback will do these structural/ordinary edits.
            // This fixture exercises its executor contract without claiming AE script qualification.
            parade.erase(std::remove_if(parade.begin(),parade.end(),[](const Ptr& e){return e->kind>=0 && e->numbers.at(guard(e->kind))!=2;}),parade.end());
            for(const auto& asset:t.assets){auto p=make(5,61);auto target=asset.node_id;if(t.wrong_target)target[0]^=1;uuid(*p,target);if(t.busy_target)p->numbers[94]=1;parade.push_back(p);
                if(t.duplicate_target)parade.push_back(std::make_shared<Effect>(*p));}
            if(t.prepare_throw)throw std::bad_alloc();return t.prepare_fail?injected:0;};
        p.commit=[](void* context)->A_Err{auto& t=*static_cast<Transaction*>(context);++t.commits;CHECK(t.depth==1);no_refs();
            if(t.commit_throw)throw std::runtime_error("commit exception");return t.commit_fail?injected:0;};
        p.finalize=[](void* context)->A_Err{auto& t=*static_cast<Transaction*>(context);++t.finalizes;CHECK(t.depth==1);no_refs();
            for(const auto& p:parade)if(p->kind>=-1)CHECK(p->numbers.at(guard(p->kind))!=transaction_backup_guard);
            CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);return t.finalize_fail?injected:0;};
        p.is_cancelled=[](void* context) noexcept->std::int32_t{const auto& t=*static_cast<Transaction*>(context);
            return t.cancel_phase==1 || (t.cancel_phase==2 && t.prepares) || (t.cancel_phase==3 && t.written>=1) || (t.cancel_phase==4 && t.written==t.assets.size());};return p;}
};
Transaction* transaction{};
Transaction make_transaction(unsigned assets=2){reset();Transaction t;
    starfield::core::NeverCancelled never;auto cube=starfield::core::make_unit_cube();CHECK(cube.has_value());
    auto encoded=starfield::core::encode_model_geometry(cube.value(),never);CHECK(encoded.has_value());
    for(auto b:encoded.value())t.bytes.push_back(std::to_integer<std::uint8_t>(b));
    for(unsigned i=0;i<assets;++i){UUID id{};id.back()=static_cast<std::uint8_t>(100+i);t.ids.push_back(id);t.assets.push_back({id,2,27+i,{},{-.5,-.5,-.5,.5,.5,.5}});}return t;}
ModelGraphTransactionResult run(Transaction& t){transaction=&t;for(auto& asset:t.assets)if(asset.revision)asset.mesh=t.bytes;const auto result=apply_model_graph_transaction(t.plan());no_owned();CHECK(t.depth==0 && t.starts==t.ends);return result;}
void transaction_tests(){
    utility.AEGP_StartUndoGroup=[](const A_char* name)->A_Err{CHECK(!std::strcmp(name,"Starfield: apply graph and Model assets"));auto& t=*transaction;no_refs();CHECK(t.depth==0);
        if(t.start_fail)return injected;++t.starts;++t.depth;return 0;};
    utility.AEGP_EndUndoGroup=[]()->A_Err{auto& t=*transaction;no_refs();CHECK(t.depth==1);--t.depth;++t.ends;return t.end_fail?injected:0;};
    effects.AEGP_EffectCallGeneric=[](AEGP_PluginID plugin,AEGP_EffectRefH h,const A_Time* time,PF_Cmd command,void* extra)->A_Err{
        auto& t=*transaction;++t.generics;CHECK(plugin==7 && time->value==12 && time->scale==24 && command==PF_Cmd_COMPLETELY_GENERAL && t.depth==1);
        auto& r=*static_cast<ModelAssetWriteRequest*>(extra);CHECK(r.magic==0x53464d57 && r.version==1 && r.bytes==sizeof(r) && r.operation==1);
        auto p=effect(h);CHECK(p->kind==5 && p->numbers.at(94)==0 && effect_refs==1 && stream_refs==0 && values==0 && handles==0 && locks==0);
        UUID expected{};std::copy(std::begin(r.expected_uuid),std::end(r.expected_uuid),expected.begin());CHECK(uuid(*p)==expected);
        CHECK(r.expected_source==p->numbers.at(1) && r.expected_revision==p->numbers.at(4));CHECK(!r.is_cancelled(r.cancellation_context));
        if(t.generic_fail_at==t.generics)return injected;
        if(t.unacknowledged)return 0;r.acknowledged=1;
        p->arbitrary[3]=r.mesh_length?std::vector<std::uint8_t>(r.mesh_bytes,r.mesh_bytes+r.mesh_length):std::vector<std::uint8_t>{};
        p->numbers[1]=r.desired_source;if(!t.silent_revision)p->numbers[4]=r.desired_revision;
        for(unsigned i=0;i<6;++i)p->numbers[95+i]=r.desired_bounds[i];
        if(t.local_failure){r.error=ModelAssetError::host_error;r.host_error=injected;r.rollback_error=512;return 0;}
        r.error=ModelAssetError::none;++t.written;return 0;};
    {auto t=make_transaction();auto r=run(t);CHECK(r.committed && r.error==0 && r.cleanup_error==0 && r.undo_error==0 && r.rollback_error==0 &&
        r.stage==ModelTransactionStage::complete && t.prepares==1 && t.commits==1 && t.generics==2);
        CHECK(parade.size()==5);for(const auto& a:t.assets){const auto p=std::find_if(parade.begin(),parade.end(),[&](const Ptr& e){return e->kind==5 && uuid(*e)==a.node_id;});
            CHECK(p!=parade.end() && (*p)->arbitrary.at(3)==t.bytes && (*p)->numbers.at(1)==2 && (*p)->numbers.at(4)==a.revision);}}
    {auto t=make_transaction(1);t.assets[0].source=1;t.assets[0].revision=0;t.bytes.clear();auto r=run(t);CHECK(r.committed && t.generics==1);}
    for(unsigned which=0;which<12;++which){auto t=make_transaction();const auto before=snapshot();
        switch(which){case 0:t.prepare_fail=true;break;case 1:t.commit_fail=true;break;case 2:t.prepare_throw=true;break;case 3:t.commit_throw=true;break;
            case 4:t.generic_fail_at=1;break;case 5:t.generic_fail_at=2;break;case 6:t.unacknowledged=true;break;case 7:t.wrong_target=true;break;
            case 8:t.duplicate_target=true;break;case 9:t.busy_target=true;break;case 10:t.silent_revision=true;break;case 11:t.local_failure=true;break;}
        const auto r=run(t);CHECK(!r.committed && r.error!=0 && r.rollback_error==0);equals(before);CHECK(t.starts==1 && t.ends==1 && t.finalizes==1);
        if(which==11)CHECK(r.asset_rollback_error==512);}
    for(unsigned phase=1;phase<=4;++phase){auto t=make_transaction();const auto before=snapshot();t.cancel_phase=phase;const auto r=run(t);
        CHECK(!r.committed && r.error==PF_Interrupt_CANCEL && r.rollback_error==0);equals(before);if(phase==1)CHECK(t.starts==0);}
    for(unsigned which=0;which<9;++which){auto t=make_transaction();const auto before=snapshot();
        switch(which){case 0:t.assets[1].node_id=t.assets[0].node_id;break;case 1:t.assets[1].source=3;break;case 2:t.assets[1].revision=2147483648u;break;
            case 3:t.assets[1].bounds[0]=-2;break;case 4:t.bytes[0]=0;break;case 5:t.assets[1].node_id={};break;
            case 6:t.ids[1]=t.ids[0];break;case 7:t.assets[1].bounds[2]=std::numeric_limits<double>::infinity();break;case 8:t.assets[1].node_id.back()=99;break;}
        const auto r=run(t);CHECK(!r.committed && r.error!=0 && t.starts==0 && t.prepares==0 && t.generics==0);equals(before);}
    {auto t=make_transaction(9);transaction=&t;const auto before=snapshot();for(auto& asset:t.assets)asset.mesh={reinterpret_cast<const std::uint8_t*>(1),8*1024*1024};
        auto r=apply_model_graph_transaction(t.plan());CHECK(r.error!=0 && t.starts==0);equals(before);no_owned();}
    {auto t=make_transaction();t.start_fail=true;const auto before=snapshot();const auto r=run(t);CHECK(r.error==injected && t.prepares==0);equals(before);}
    {auto t=make_transaction();missing_suite=kAEGPUtilitySuite;const auto before=snapshot();const auto r=run(t);CHECK(r.error!=0 && t.starts==0);equals(before);}
    {auto t=make_transaction();inject("duplicate",2);const auto before=snapshot();const auto r=run(t);CHECK(r.error==injected && t.prepares==0 && r.rollback_error==0);equals(before);}
    {auto t=make_transaction();t.commit_fail=true;inject("delete",1);const auto r=run(t);CHECK(!r.committed && r.error==injected && r.rollback_error==injected);}
    {auto t=make_transaction();inject("delete",1);const auto r=run(t);CHECK(r.committed && r.error==0 && r.cleanup_error==injected && t.commits==1);}
    {auto t=make_transaction();t.end_fail=true;const auto r=run(t);CHECK(r.committed && r.error==0 && r.undo_error==injected);}
    {auto t=make_transaction();t.finalize_fail=true;const auto r=run(t);CHECK(r.committed && r.error==0 && r.cleanup_error==injected && t.finalizes==1);}
    {auto t=make_transaction();t.commit_fail=t.finalize_fail=true;const auto r=run(t);
        CHECK(!r.committed && r.error==injected && r.rollback_error==injected && t.finalizes==1);
        CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);}
    transaction=nullptr;
}
#endif
}
int main(){initialize();success_tests();preflight_tests();prepare_failure_tests();restore_failure_tests();final_parade_tests();
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
    transaction_tests();std::cout<<"Model whole-graph transaction: "<<checks<<" checks, 0 failures (actual backup/SFMG1, fake May2023 callbacks)\n";
#else
    std::cout<<"Complete effect graph backup: "<<checks<<" checks, 0 failures (May2023 SDK/fake host)\n";
#endif
    return 0;}
