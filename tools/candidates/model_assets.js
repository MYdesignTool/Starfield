// Candidate-only portable numeric Model assets and explicit export transport.
(function(root,factory){var api=factory();if(typeof module==="object" && module.exports)module.exports=api;
    else root.StarfieldModelAssets=api;}(typeof window!=="undefined"?window:this,function(){
    "use strict";
    var MODEL="org.starfieldfx.nodes.model",MAX_MESH=8*1024*1024,MAX_TOTAL=64*1024*1024,PAGE_HEX=65536;
    var crcTable=new Uint32Array(256);
    for(var table=0;table<256;table++){var crc=table;for(var bit=0;bit<8;bit++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);crcTable[table]=crc>>>0;}
    function fail(message){var error=new Error(message);error.code="invalid_model_asset";throw error;}
    function identifier(value){return typeof value==="string" && /^[0-9a-f]{32}$/.test(value) && !/^0{32}$/.test(value);}
    function integer(value,min,max){return typeof value==="number" && isFinite(value) && Math.floor(value)===value && value>=min && value<=max;}
    function same(a,b){return Array.isArray(a) && Array.isArray(b) && a.length===b.length && a.every(function(v,i){return v===b[i];});}
    function mesh(hex){
        if(typeof hex!=="string" || hex.length<64 || hex.length>MAX_MESH*2 || hex.length%2 || !/^[0-9a-f]+$/.test(hex))fail("Invalid Model mesh hex payload.");
        var bytes=new Uint8Array(hex.length/2);
        for(var i=0;i<bytes.length;i++)bytes[i]=parseInt(hex.substr(i*2,2),16);
        var data=new DataView(bytes.buffer),positions=data.getUint32(16,true),uvs=data.getUint32(20,true),normals=data.getUint32(24,true),triangles=data.getUint32(28,true);
        if(data.getUint32(0,true)!==0x474d4653 || data.getUint16(4,true)!==1 || data.getUint16(6,true)!==32 || data.getUint32(8,true)!==bytes.length ||
            !integer(positions,1,65536) || !integer(triangles,1,65536) || !integer(uvs,0,65536) || !integer(normals,0,65536) ||
            32+32*positions+24*uvs+24*normals+36*triangles!==bytes.length)fail("Invalid SFMG1 Model mesh header.");
        var crc=0xffffffff;
        for(var offset=32;offset<bytes.length;offset++)crc=(crc>>>8)^crcTable[(crc^bytes[offset])&255];
        if(((crc^0xffffffff)>>>0)!==data.getUint32(12,true))fail("Model mesh checksum mismatch.");
        function scalar(offset){var value=data.getFloat64(offset,true);if(!isFinite(value) || Math.abs(value)>1e9)fail("Invalid Model numeric coordinate.");return value;}
        for(var p=0;p<positions;p++)for(var field=0;field<4;field++)scalar(32+p*32+field*8);
        var uvStart=32+32*positions,normalStart=uvStart+24*uvs,triangleStart=normalStart+24*normals;
        for(var attribute=uvStart;attribute<triangleStart;attribute+=24){
            var x=scalar(attribute),y=scalar(attribute+8),z=scalar(attribute+16);
            if(attribute>=normalStart && x===0 && y===0 && z===0)fail("Invalid zero Model normal.");
        }
        var bounds=[Infinity,Infinity,Infinity,-Infinity,-Infinity,-Infinity];
        for(var t=0;t<triangles;t++){
            var corners=[];
            for(var corner=0;corner<3;corner++){
                var at=triangleStart+t*36+corner*12,index=data.getUint32(at,true),uv=data.getUint32(at+4,true),normal=data.getUint32(at+8,true);
                if(index>=positions || (uv!==0xffffffff && uv>=uvs) || (normal!==0xffffffff && normal>=normals))fail("Invalid Model corner index.");
                var point=[];for(var axis=0;axis<3;axis++){var coordinate=data.getFloat64(32+index*32+axis*8,true);point.push(coordinate);
                    bounds[axis]=Math.min(bounds[axis],coordinate);bounds[axis+3]=Math.max(bounds[axis+3],coordinate);}corners.push(point);
            }
            var u=corners[1].map(function(v,i){return v-corners[0][i];}),v=corners[2].map(function(n,i){return n-corners[0][i];});
            var extent=Math.max.apply(null,u.concat(v).map(Math.abs));
            if(extent<=0)fail("Degenerate Model triangle.");
            u=u.map(function(n){return n/extent;});v=v.map(function(n){return n/extent;});
            if(Math.max(Math.abs(u[1]*v[2]-u[2]*v[1]),Math.abs(u[2]*v[0]-u[0]*v[2]),Math.abs(u[0]*v[1]-u[1]*v[0]))<=1e-12)fail("Degenerate Model triangle.");
        }
        return {bytes:bytes.length,bounds:bounds,positions:positions,textureCoordinates:uvs,normals:normals,triangles:triangles};
    }
    function parameter(node,key,type){var found=(node.parameters || []).filter(function(p){return String(p.key)===String(key);});
        if(found.length!==1 || found[0].type!==type)fail("Model graph field is missing or duplicated: "+key);return found[0].value;}
    function graphBounds(node){var raw=parameter(node,12,7);if(!raw || raw.length!==48)fail("Invalid Model graph bounds.");
        for(var i=0;i<raw.length;i++)if(!integer(raw[i],0,255))fail("Invalid Model graph bounds byte.");
        var data=new DataView(new Uint8Array(raw).buffer),bounds=[];for(var axis=0;axis<6;axis++)bounds.push(data.getFloat64(axis*8,true));return bounds;}
    function validate(graph,assets){
        if(!graph || !Array.isArray(graph.nodes) || graph.nodes.length>64 || !Array.isArray(assets) || assets.length>256)fail("Invalid Model asset inventory.");
        var models={},seen={},total=0,result=[];
        graph.nodes.forEach(function(node){if(node.type!==MODEL)return;if(!identifier(node.id) || models[node.id])fail("Invalid Model graph identity.");models[node.id]=node;
            var source=parameter(node,13,3),revision=parameter(node,2,3),resource=parameter(node,1,7),bounds=graphBounds(node);
            if(!integer(source,0,1) || !integer(revision,0,2147483647) || !resource || resource.length!==16)fail("Invalid Model graph resource.");
            for(var i=0;i<16;i++)if(!integer(resource[i],0,255) || resource[i]!== (source===1 && revision>0?parseInt(node.id.substr(i*2,2),16):0))fail("Model resource identity does not match its node.");
            if((source===0 || revision===0) && (revision!==0 || !same(bounds,[-.5,-.5,-.5,.5,.5,.5])))fail("Invalid default Model resource.");
        });
        assets.forEach(function(asset){
            if(!asset || !identifier(asset.nodeId) || !models[asset.nodeId] || seen[asset.nodeId] || !integer(asset.revision,1,2147483647))fail("Invalid or duplicate Model preset asset.");
            // Preflight total before decoding/allocating the next payload.
            if(typeof asset.meshHex!=="string" || asset.meshHex.length/2>MAX_TOTAL-total)fail("Model preset assets exceed the 64 MiB limit.");
            var geometry=mesh(asset.meshHex);total+=geometry.bytes;
            if(!same(asset.bounds,geometry.bounds))fail("Model asset bounds do not match its mesh.");
            var node=models[asset.nodeId],source=parameter(node,13,3);
            if(source===1 && (parameter(node,2,3)!==asset.revision || !same(graphBounds(node),geometry.bounds)))fail("Imported Model graph metadata does not match its asset.");
            seen[asset.nodeId]=true;result.push({nodeId:asset.nodeId,revision:asset.revision,bounds:geometry.bounds.slice(),meshHex:asset.meshHex});
        });
        Object.keys(models).forEach(function(id){if(parameter(models[id],13,3)===1 && parameter(models[id],2,3)>0 && !seen[id])fail("Imported Model has no portable mesh asset.");});
        return result;
    }
    function create(options){
        options=options || {};if(typeof options.call!=="function")throw new Error("Model asset client needs a gateway call.");
        var schedule=options.schedule || function(fn,delay){return setTimeout(fn,delay);},clock=options.now || Date.now;
        var idFactory=options.idFactory || function(){var id="";for(var i=0;i<32;i++)id+=Math.floor(Math.random()*16).toString(16);return id;};
        function collect(snapshot,token,callback){
            if(!snapshot || !integer(snapshot.revision,1,16777215) || typeof token!=="string" || typeof callback!=="function")throw new Error("Model export needs a captured snapshot and pinned target.");
            var models=(snapshot.nativeNodes || []).filter(function(n){return n.type===MODEL && n.modelAsset && n.modelAsset.revision>0;}),
                assets=[],index=0,current=null,finished=false,total=0,ids={};
            function release(done){if(!current){done();return;}var id=current;current=null;options.call("releaseModelAsset",{assetId:id,pinTarget:true,target:{token:token}},function(){done();});}
            function end(response){if(finished)return;finished=true;release(function(){callback(response);});}
            function error(value){end({ok:false,error:value && value.code?value:{code:"model_asset_transfer_failed",message:String(value && value.message || value)}});}
            function next(){
                if(finished)return;if(index===models.length){end({ok:true,assets:assets});return;}
                var node=models[index++],id=idFactory(),pages=[],metadata=null,deadline=clock()+60000;
                if(!identifier(id) || ids[id]){error("Could not allocate a Model transfer identity.");return;}ids[id]=true;current=id;
                var fields={assetId:id,nodeId:node.id,baseGraphRevision:snapshot.revision,pinTarget:true,target:{token:token}};
                options.call("beginModelAssetExport",fields,function(response){
                    if(finished)return;if(!response || !response.ok){error(response && response.error || "No Model export response.");return;}read();
                });
                function read(){
                    if(finished)return;if(clock()>deadline){error({code:"model_asset_timeout",message:"Model asset transfer timed out."});return;}
                    options.call("readModelAssetPage",{assetId:id,page:pages.length,pinTarget:true,target:{token:token}},function(response){
                        if(finished)return;
                        if(!response || !response.ok){error(response && response.error || "No Model page response.");return;}
                        if(response.assetId!==id){error("Model transfer identity changed.");return;}
                        if(response.state==="queued" || response.state==="receiving"){schedule(read,50);return;}
                        try {
                            if(response.state!=="ready" || response.page!==pages.length || response.nodeId!==node.id ||
                                !integer(response.bytes,32,MAX_MESH) || response.pageCount!==Math.ceil(response.bytes*2/PAGE_HEX) ||
                                response.source!==parameter(node,13,3)+1 || response.revision!==node.modelAsset.revision || !same(response.bounds,node.modelAsset.bounds) ||
                                typeof response.hex!=="string" || response.hex.length!==Math.min(PAGE_HEX,response.bytes*2-pages.length*PAGE_HEX) || !/^[0-9a-f]+$/.test(response.hex))fail("Invalid Model page metadata or length.");
                            if(metadata && (metadata.bytes!==response.bytes || metadata.pageCount!==response.pageCount))fail("Model payload changed between pages.");
                            metadata=response;pages.push(response.hex);
                            if(pages.length!==metadata.pageCount){read();return;}
                            if(metadata.bytes>MAX_TOTAL-total)fail("Model preset assets exceed the 64 MiB limit.");
                            var hex=pages.join(""),geometry=mesh(hex);
                            if(!same(geometry.bounds,node.modelAsset.bounds))fail("Model payload does not match captured bounds.");
                            total+=geometry.bytes;assets.push({nodeId:node.id,revision:node.modelAsset.revision,bounds:geometry.bounds,meshHex:hex});
                            release(next);
                        }catch(failure){error(failure);}
                    });
                }
            }
            next();return {cancel:function(){end({ok:false,error:{code:"cancelled",message:"Model asset transfer cancelled."}});}};
        }
        return {collect:collect};
    }
    return {validateMesh:mesh,validate:validate,create:create,maxMeshBytes:MAX_MESH,maxTotalBytes:MAX_TOTAL};
}));
