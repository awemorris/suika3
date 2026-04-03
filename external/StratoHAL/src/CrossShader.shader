Shader "StratoHAL/CrossShader"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _Src2Tex ("Texture", 2D) = "white" {}
    }

    SubShader
    {
        Tags
        {
            "Queue"="Transparent"
            "RenderType"="Transparent"
            "IgnoreProjector"="True"
        }

        LOD 100
        ZWrite Off
        //ZTest Always
        Blend SrcAlpha OneMinusSrcAlpha
        BlendOp Add

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv1 : TEXCOORD0;
                float2 uv2 : TEXCOORD1;
                float4 color : COLOR;
            };

            struct v2f
            {
                float2 uv1 : TEXCOORD0;
                float2 uv2 : TEXCOORD1;
                float4 vertex : SV_POSITION;
                float4 color : COLOR;
            };

            sampler2D _MainTex;
            sampler2D _Src2Tex;
            float4 _MainTex_ST;
            float4 _Src2Tex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = v.vertex;
                o.uv1 = v.uv1;
                o.uv2 = v.uv2;
                o.color = v.color;
                return o;
            }

            float4 frag (v2f i) : SV_Target
            {
                float4 col1 = tex2D(_MainTex, i.uv1);
                float4 col2 = tex2D(_Src2Tex, i.uv2);
                return col1 * i.color.a + col2 * (1.0 - i.color.a);
            }
            ENDCG
        }
    }
}
