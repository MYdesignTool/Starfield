// Included by GraphEvaluation.cpp; numeric Model metadata only, never mesh bytes.
using ModelValues=std::map<NodeId,ParticleModelInstance>;
using ModelStyleIndices=std::map<NodeId,std::uint32_t>;
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
    return source.type_key==kModelNode && destination.type_key==kParticleNode &&
        edge.source_port==kModelGeometryOut && edge.destination_port==kParticleModelsIn;
}
Result<ModelStyleIndices> retain_model_styles(const Graph& graph,const ModelValues& models,EvaluatedGraph& result,
    const Cancellation& cancel) {
    using R=Result<ModelStyleIndices>;
    std::map<NodeId,std::vector<NodeId>> inputs;
    for(const auto& edge:graph.edges)if(models.contains(edge.source_node))inputs[edge.destination_node].push_back(edge.source_node);
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
