#ifndef LITTLE_HELPERS_TERMINAL_MODEL_H
#define LITTLE_HELPERS_TERMINAL_MODEL_H
/* Editable metre-scale assembly. +Z faces the room; the two action pads keep
 * their existing collider identities and purchase handlers in terminal.h. */
typedef struct {float x,y,z;} LTPoint;
typedef struct {const char *name;LTPoint center,size;float corner,bevel;unsigned material;} LTPart;
static const LTPart lt_parts[]={
    {"Wall mounting gasket",{0,0,.020f},{1.82f,1.38f,.040f},.09f,.008f,1},
    {"Chamfered ceramic case",{0,0,.094f},{1.85f,1.40f,.130f},.10f,.025f,0},
    {"Inset control fascia",{0,-.005f,.164f},{1.69f,1.22f,.034f},.055f,.009f,1},
    {"Status display",{0,.235f,.187f},{1.53f,.47f,.022f},.025f,.005f,2},
    {"Orange identification rail",{0,.566f,.188f},{1.45f,.037f,.018f},.012f,.004f,3},
    {"Lower trim",{0,-.557f,.186f},{1.45f,.020f,.016f},.006f,.003f,0},
    {"Buy button bezel",{.43f,-.295f,.195f},{.765f,.285f,.040f},.035f,.009f,0},
    {"Upgrade button bezel",{-.43f,-.295f,.195f},{.765f,.285f,.040f},.035f,.009f,0},
    {"Left mounting screw",{-.836f,.59f,.165f},{.038f,.038f,.014f},.010f,.003f,1},
    {"Right mounting screw",{.836f,.59f,.165f},{.038f,.038f,.014f},.010f,.003f,1},
    {"Left lower screw",{-.836f,-.59f,.165f},{.038f,.038f,.014f},.010f,.003f,1},
    {"Right lower screw",{.836f,-.59f,.165f},{.038f,.038f,.014f},.010f,.003f,1}
};
static const LTPart lt_buttons[]={
    {"Buy a plush helper",{.43f,-.295f,.226f},{.70f,.22f,.050f},.025f,.008f,3},
    {"Upgrade helper capacity",{-.43f,-.295f,.226f},{.70f,.22f,.050f},.025f,.008f,3}
};
#define LT_VERTEX_LIMIT 4096
#define LT_INDEX_LIMIT 6144
typedef struct {LTPoint positions[LT_VERTEX_LIMIT],normals[LT_VERTEX_LIMIT];int indices[LT_INDEX_LIMIT];unsigned vertices,indices_count;} LTMesh;
static float lt_abs(float x){return x<0?-x:x;}
static LTPoint lt_sub(LTPoint a,LTPoint b){return (LTPoint){a.x-b.x,a.y-b.y,a.z-b.z};}
static int lt_triangle(LTMesh *m,LTPoint a,LTPoint b,LTPoint c){
    if(m->vertices+3>LT_VERTEX_LIMIT||m->indices_count+3>LT_INDEX_LIMIT)return 0;
    LTPoint x=lt_sub(b,a),y=lt_sub(c,a),n={x.y*y.z-x.z*y.y,x.z*y.x-x.x*y.z,x.x*y.y-x.y*y.x};
    float max=lt_abs(n.x);if(lt_abs(n.y)>max)max=lt_abs(n.y);if(lt_abs(n.z)>max)max=lt_abs(n.z);if(max<1e-12f)return 0;
    n.x/=max;n.y/=max;n.z/=max;float q=n.x*n.x+n.y*n.y+n.z*n.z,s=1;for(int i=0;i<8;i++)s=.5f*(s+q/s);n.x/=s;n.y/=s;n.z/=s;
    unsigned v=m->vertices;m->positions[v]=a;m->positions[v+1]=b;m->positions[v+2]=c;
    for(unsigned i=0;i<3;i++)m->normals[v+i]=n;
    /* Match the pinned Unity cube readback: geometric cross product and
     * supplied outward normal have positive dot product. */
    m->indices[m->indices_count++]=(int)v;m->indices[m->indices_count++]=(int)v+1;m->indices[m->indices_count++]=(int)v+2;m->vertices+=3;return 1;
}
static int lt_shape(LTMesh *m,LTPart p){
    LTPoint ring[4][8];float w=p.size.x*.5f,h=p.size.y*.5f,d=p.size.z*.5f,b=p.bevel,c=p.corner;
    if(!(w>c&&h>c&&c>b&&d>b&&b>0))return 0;
    const float xy[8][2]={{-1,-1},{1,-1},{1,-1},{1,1},{1,1},{-1,1},{-1,1},{-1,-1}};
    for(unsigned r=0;r<4;r++)for(unsigned i=0;i<8;i++){
        float inset=r==0||r==3?b:0,xx=w-inset,yy=h-inset,cut=c-inset;
        float x=xy[i][0]*xx,y=xy[i][1]*yy;
        if(i==0||i==1||i==4||i==5)x-=xy[i][0]*cut;else y-=xy[i][1]*cut;
        float z=r==0?-d:r==1?-d+b:r==2?d-b:d;
        ring[r][i]=(LTPoint){p.center.x+x,p.center.y+y,p.center.z+z};
    }
    for(unsigned r=0;r<3;r++)for(unsigned i=0;i<8;i++){unsigned j=(i+1)%8;if(!lt_triangle(m,ring[r][i],ring[r][j],ring[r+1][j])||!lt_triangle(m,ring[r][i],ring[r+1][j],ring[r+1][i]))return 0;}
    LTPoint back=p.center,front=p.center;back.z-=d;front.z+=d;
    for(unsigned i=0;i<8;i++){unsigned j=(i+1)%8;if(!lt_triangle(m,front,ring[3][i],ring[3][j])||!lt_triangle(m,back,ring[0][j],ring[0][i]))return 0;}
    return 1;
}
#endif
