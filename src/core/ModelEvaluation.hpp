// Included by GraphEvaluation.cpp; numeric Model metadata only, never mesh bytes.
using ModelValues=std::map<NodeId,ParticleModelInstance>;
using ModelStyleIndices=std::map<NodeId,std::uint32_t>;
using MaterialValues=std::map<NodeId,ModelMaterial>;
Result<ModelMaterial> read_material(const GraphNode& node) {
    using R=Result<ModelMaterial>;ModelMaterial m;m.enabled=true;
    if(node.type_key!=kMaterialNode||node.schema_version!=1||node.parameters.size()!=6)
        return R::failure(ErrorCode::invalid_request,"invalid Material schema");
    std::array<bool,6> seen{};
    for(const auto& p:node.parameters){const auto k=p.key.value;
        if(k<1||k>6||seen[k-1])return R::failure(ErrorCode::invalid_request,"invalid Material key");seen[k-1]=true;
        if(k==1||k==6){const auto* v=std::get_if<std::uint32_t>(&p.value);if(!v||*v>1)return R::failure(ErrorCode::invalid_request,"invalid Material switch");
            if(k==1)m.color_source=*v;else m.ae_lights=*v!=0;}
        else if(k==2){const auto* v=std::get_if<Vec3>(&p.value);if(!v)return R::failure(ErrorCode::invalid_request,"invalid Material color");m.color=*v;}
        else {const auto* v=std::get_if<double>(&p.value);if(!v)return R::failure(ErrorCode::invalid_request,"invalid Material scalar");
            if(k==3)m.roughness=*v;else if(k==4)m.metallic=*v;else m.opacity=*v;}
    }
    if(!valid_model_material(m))return R::failure(ErrorCode::invalid_request,"Material value outside bounds");return R::success(m);
}
Result<bool> attach_materials(const Graph& graph,const MaterialValues& materials,ModelValues& models) {
    // Retain the deployed metadata attachment when reading an old project.
    for(const auto& edge:graph.edges)if(const auto found=materials.find(edge.source_node);found!=materials.end() &&
        models.contains(edge.destination_node) && edge.destination_port==kModelMaterialIn) {
        if(edge.source_port!=kMaterialOut)return Result<bool>::failure(ErrorCode::invalid_request,"invalid legacy Material output");
        models.at(edge.destination_node).material=found->second;
    }
    // A downstream Material owns a shaded geometry branch, not the upstream
    // Model's shared instance. Fan-out can therefore use independent colors.
    for(std::size_t pass=0;pass<materials.size();++pass) {
        bool progress=false;
        for(const auto& edge:graph.edges)if(materials.contains(edge.destination_node) &&
            edge.destination_port==kMaterialGeometryIn && !models.contains(edge.destination_node)) {
            const auto source=models.find(edge.source_node);
            if(source==models.end())continue;
            auto instance=source->second;instance.material=materials.at(edge.destination_node);
            models.emplace(edge.destination_node,std::move(instance));progress=true;
        }
        if(!progress)break;
    }
    return Result<bool>::success(true);
}
Result<ParticleModelInstance> read_model(const GraphNode& node) {
    using R=Result<ParticleModelInstance>;
    if(node.type_key!=kModelNode || node.schema_version!=1)return R::failure(ErrorCode::invalid_request,"invalid Model node identity/schema");
    ParticleModelInstance instance;std::array<bool,13> seen{};ModelLocalSettings pose;
    ModelBounds bounds{{-.5,-.5,-.5},{.5,.5,.5}};bool authored_pose=false;std::uint32_t source=0;
    for(const auto& parameter:node.parameters) {
        const auto key=parameter.key.value;
        if(key<1 || key>seen.size() || seen[key-1])return R::failure(ErrorCode::invalid_request,"unknown or duplicate Model parameter");
        seen[key-1]=true;
        if(parameter.key==kModelRevision) {
            if(!std::get_if<std::uint32_t>(&parameter.value))return R::failure(ErrorCode::invalid_request,"invalid Model revision kind");
            continue;
        }
        if(key>=4 && key<=6) {
            const auto* value=std::get_if<Vec3>(&parameter.value);if(!value)return R::failure(ErrorCode::invalid_request,"invalid Model pose vector kind");
            if(key==4)pose.origin=*value;else if(key==5)pose.rotation_degrees=*value;else pose.scale_percent=*value;
            authored_pose=true;continue;
        }
        if((key>=7 && key<=11)||key==13) {
            const auto* value=std::get_if<std::uint32_t>(&parameter.value);
            if(!value||*value>1)return R::failure(ErrorCode::invalid_request,"invalid Model author enum/bool");
            if(key==13)source=*value;
            else {bool* fields[]{&pose.flip_x,&pose.flip_y,&pose.flip_z,&pose.center,&pose.normalize};*fields[key-7]=*value!=0;authored_pose=true;}
            continue;
        }
        const auto* bytes=std::get_if<OpaqueBytes>(&parameter.value);
        if(!bytes || bytes->size()!=(parameter.key==kModelResource?16u:key==12?48u:128u))
            return R::failure(ErrorCode::invalid_request,"invalid Model metadata byte length/kind");
        if(parameter.key==kModelResource)for(unsigned i=0;i<16;++i)instance.resource[i]=std::to_integer<std::uint8_t>((*bytes)[i]);
        else for(unsigned i=0;i<(key==12?6u:16u);++i) {
            std::uint64_t bits=0;for(unsigned b=0;b<8;++b)bits|=std::uint64_t(std::to_integer<std::uint8_t>((*bytes)[i*8+b]))<<(8*b);
            const auto value=std::bit_cast<double>(bits);
            if(key==12){double* fields[]{&bounds.minimum.x,&bounds.minimum.y,&bounds.minimum.z,&bounds.maximum.x,&bounds.maximum.y,&bounds.maximum.z};*fields[i]=value;}
            else instance.model_to_particle[i]=value;
        }
    }
    if(seen[12]&&source==0&&instance.resource!=ModelResourceId{})return R::failure(ErrorCode::invalid_request,"cube Model source cannot select an imported resource");
    if(authored_pose||seen[11]) {
        if(authored_pose&&seen[2])return R::failure(ErrorCode::invalid_request,"Model authored pose conflicts with explicit matrix");
        auto matrix=model_local_matrix(pose,bounds);if(!matrix.has_value())return R::failure(matrix.error());
        if(authored_pose)instance.model_to_particle=matrix.value();
    }
    if(!valid_model_instance(instance))return R::failure(ErrorCode::invalid_request,"Model local matrix must be finite bounded affine");
    return R::success(instance);
}
bool is_model_graph_edge(const GraphEdge& edge,const GraphNode& source,const GraphNode& destination) noexcept {
    return ((source.type_key==kModelNode || source.type_key==kMaterialNode) && edge.source_port==kModelGeometryOut &&
        ((destination.type_key==kParticleNode && edge.destination_port==kParticleModelsIn) ||
         (destination.type_key==kMaterialNode && edge.destination_port==kMaterialGeometryIn) ||
         (destination.type_key==kOutputNode && edge.destination_port==kOutputParticles))) ||
        (source.type_key==kMaterialNode && destination.type_key==kModelNode &&
         edge.source_port==kMaterialOut && edge.destination_port==kModelMaterialIn);
}
Result<ModelStyleIndices> retain_model_styles(const Graph& graph,const ModelValues& models,EvaluatedGraph& result,
    const Cancellation& cancel) {
    using R=Result<ModelStyleIndices>;
    std::map<NodeId,std::vector<NodeId>> inputs;
    for(const auto& edge:graph.edges)if(models.contains(edge.source_node)) {
        const auto target=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==edge.destination_node;});
        if(target!=graph.nodes.end() && (target->type_key==kParticleNode || target->type_key==kOutputNode))
            inputs[edge.destination_node].push_back(edge.source_node);
    }
    ModelStyleIndices indices;
    for(auto& [particle,members]:inputs) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Model graph grouping cancelled");
        if(members.empty() || members.size()>kMaxModelsPerStyle || result.model_styles.size()>=kMaxModelStyles)
            return R::failure(ErrorCode::work_limit_exceeded,"Model graph group cap exceeded");
        std::sort(members.begin(),members.end());
        if(std::adjacent_find(members.begin(),members.end())!=members.end())return R::failure(ErrorCode::invalid_request,"duplicate Model input connection");
        ParticleModelStyle style;style.instances.reserve(members.size());
        for(auto id:members)style.instances.push_back(models.at(id));
        result.model_styles.push_back(std::move(style));indices.emplace(particle,static_cast<std::uint32_t>(result.model_styles.size()));
    }
    return R::success(std::move(indices));
}
Result<EvaluatedGraph> finish_model_output(EvaluatedGraph result,const Graph& graph,const ModelValues& models,
    const ModelStyleIndices& indices) {
    using R=Result<EvaluatedGraph>;
    for(const auto& node:graph.nodes)if(node.type_key==kOutputNode) {
        const auto found=indices.find(node.id);if(found==indices.end())continue;
        for(const auto& edge:graph.edges)if(edge.destination_node==node.id && models.contains(edge.source_node)) {
            if(std::any_of(result.particles.begin(),result.particles.end(),[&](const auto& p){return p.emitter_id==edge.source_node && p.id==UINT64_MAX;}))continue;
            if(result.particles.size()>=kMaxParticleCount)return R::failure(ErrorCode::work_limit_exceeded,"direct Model output count exceeded");
            // One geometry group per direct model: do not duplicate the combined group for every edge.
            ParticleModelStyle style;style.instances.push_back(models.at(edge.source_node));
            if(result.model_styles.size()>=kMaxModelStyles)return R::failure(ErrorCode::work_limit_exceeded,"direct Model styles exceeded");
            result.model_styles.push_back(std::move(style));
            ParticleInstance p;p.emitter_id=edge.source_node;p.id=UINT64_MAX;p.lifetime_seconds=1e30;p.shape=4;
            p.size_pixels=models.at(edge.source_node).resource==ModelResourceId{}?100:1;p.size_y_pixels=p.size_pixels;p.opacity=1;
            p.model_style_index=static_cast<std::uint32_t>(result.model_styles.size());result.particles.push_back(p);
        }
    }
    return R::success(std::move(result));
}
