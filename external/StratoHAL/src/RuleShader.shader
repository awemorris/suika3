Shader "NoctVM/RuleShader"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _RuleTex ("Texture", 2D) = "white" {}
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
                float2 uv : TEXCOORD0;
                float4 color : COLOR;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                float4 vertex : SV_POSITION;
                float4 color : COLOR;
            };

            sampler2D _MainTex;
            sampler2D _RuleTex;
            float4 _MainTex_ST;
            float4 _RuleTex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = v.vertex;
                o.uv = v.uv;
                o.color = v.color;
                return o;
            }

            float4 frag (v2f i) : SV_Target
            {
                float4 col1 = tex2D(_MainTex, i.uv);
                float4 col2 = tex2D(_RuleTex, i.uv);
                col1.a = 1.0 - step(i.color.a, col2.b);
                return col1;
            }
            ENDCG
        }
    }
}
