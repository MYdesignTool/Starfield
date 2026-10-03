// Independent scalar kernel, compiled as CUDA and OpenCL. No graphics context.
// Layout agrees with SfGpuSprite (80 bytes). One thread owns one output pixel;
// tile indices preserve compositing order, so no floating-point atomics are used.
#if defined(SF_CUDA)
#define SF_GLOBAL
#define SF_KERNEL extern "C" __global__
#define SF_INLINE __device__ inline
#define SF_X (blockIdx.x * blockDim.x + threadIdx.x)
#define SF_Y (blockIdx.y * blockDim.y + threadIdx.y)
#else
#define SF_GLOBAL __global
#define SF_KERNEL __kernel
#define SF_INLINE inline
#define SF_X get_global_id(0)
#define SF_Y get_global_id(1)
#endif
typedef unsigned int sf_uint;
typedef struct {
    float x,y,ia,ib,ic,id,edge,feather,r,g,b,opacity;
    int left,top,right,bottom;
    sf_uint shape,reserved[3];
} SfSprite;
SF_INLINE float sf_clamp(float v) { return fminf(1.f, fmaxf(0.f,v)); }
SF_INLINE float sf_circle(float d, float scale, float feather) {
    float aa=sf_clamp(.5f+(1.f-d)*scale);
    return aa*(feather>0.f?sf_clamp((1.f-d)/feather):1.f);
}
SF_INLINE float sf_coverage(sf_uint shape,float x,float y,float edge,float feather) {
    if(shape==1) return sf_circle(fmaxf(fabsf(x),fabsf(y)),edge,feather);
    if(shape==0) return sf_circle(sqrtf(x*x+y*y),edge,feather);
    float remaining=1.f;
    const float cx[5]={0.f,-.35f,.35f,-.2f,.2f};
    const float cy[5]={0.f,-.2f,-.2f,.35f,.35f};
    for(int i=0;i<5;i++) {
        float dx=x-cx[i],dy=y-cy[i];
        remaining*=1.f-sf_circle(sqrtf(dx*dx+dy*dy)/.6f,edge*.6f,feather);
    }
    return 1.f-remaining;
}
SF_KERNEL void starfield_sprite_render(SF_GLOBAL float* output,
    SF_GLOBAL const SfSprite* sprites,SF_GLOBAL const sf_uint* offsets,
    SF_GLOBAL const sf_uint* indices,sf_uint width,sf_uint height,
    sf_uint row_floats,sf_uint tiles_x,sf_uint first_y,sf_uint roi_x,sf_uint roi_y,
    sf_uint roi_width,sf_uint roi_height,sf_uint straight) {
    sf_uint px=SF_X,py=SF_Y+first_y;
    if(px>=width || py>=height) return;
    float r=0.f,g=0.f,b=0.f,a=0.f;
    if(px>=roi_x && py>=roi_y && px-roi_x<roi_width && py-roi_y<roi_height) {
        sf_uint x=px-roi_x,y=py-roi_y;
        sf_uint tile=(y/16)*tiles_x+x/16;
        for(sf_uint k=offsets[tile];k<offsets[tile+1];k++) {
            SfSprite s=sprites[indices[k]];
            if(x<(sf_uint)s.left || y<(sf_uint)s.top || x>=(sf_uint)s.right || y>=(sf_uint)s.bottom) continue;
            float dx=(float)x+.5f-s.x,dy=(float)y+.5f-s.y;
            float alpha=sf_coverage(s.shape,dx*s.ia+dy*s.ib,dx*s.ic+dy*s.id,s.edge,s.feather)*s.opacity;
            if(alpha<=0.f) continue;
            float rem=1.f-alpha;
            r=s.r*alpha+r*rem;g=s.g*alpha+g*rem;b=s.b*alpha+b*rem;a=alpha+a*rem;
        }
    }
    if(straight && a>0.f) {r/=a;g/=a;b/=a;}
    // AE's GPU format is BGRA128, unlike CPU PF_PixelFloat's ARGB layout.
    unsigned long long address=(unsigned long long)py*row_floats+(unsigned long long)px*4;
    output[address]=b;output[address+1]=g;output[address+2]=r;output[address+3]=a;
}
// Control node effects preserve their input on the same AE GPU device. Crop by
// world origins; do not turn a metadata-only node into a CPU image transfer.
SF_KERNEL void starfield_copy_pixels(SF_GLOBAL float* output,SF_GLOBAL const float* input,
    sf_uint width,sf_uint height,sf_uint output_row,sf_uint input_width,sf_uint input_height,
    sf_uint input_row,int offset_x,int offset_y,sf_uint first_y) {
    sf_uint x=SF_X,y=SF_Y+first_y;
    if(x>=width || y>=height) return;
    int ix=(int)x+offset_x,iy=(int)y+offset_y;
    unsigned long long destination=(unsigned long long)y*output_row+(unsigned long long)x*4;
    if(ix>=0 && iy>=0 && (sf_uint)ix<input_width && (sf_uint)iy<input_height) {
        unsigned long long source=(unsigned long long)iy*input_row+(unsigned long long)ix*4;
        for(int channel=0;channel<4;channel++) output[destination+channel]=input[source+channel];
    } else for(int channel=0;channel<4;channel++) output[destination+channel]=0.f;
}
