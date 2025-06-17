
#define BIND_GROUP_GLOBAL   0
#define BIND_GROUP_MATERIAL 1
#define BIND_GROUP_DYNAMIC  2

#define BINDING_GLOBALS           0
#define BINDING_SAMPLER           1
#define BINDING_ENTITIES          2
#define BINDING_SHADOWMAP         3
#define BINDING_SHADOWMAP_SAMPLER 4

#define BINDING_MATERIAL 0
#define BINDING_ALBEDO   1

// register( name<binding>, space<descriptor set> )
#define REGISTER(reg, set, binding) register(reg##binding, space##set)

// Types of registers:
// t – for shader resource views (SRV)
// s – for samplers
// u – for unordered access views (UAV)
// b – for constant buffer views (CBV)
#define REGISTER_B(set, binding) REGISTER(b, set, binding)
#define REGISTER_S(set, binding) REGISTER(s, set, binding)
#define REGISTER_T(set, binding) REGISTER(t, set, binding)
#define REGISTER_U(set, binding) REGISTER(u, set, binding)

// Stub variable so this file debug info does not disappear in RenderDoc
static const int stubGlobalVar = 0;

