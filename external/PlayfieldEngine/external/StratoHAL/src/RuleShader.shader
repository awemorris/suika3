Shader "NoctVM/RuleShader"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _RuleTex ("Rule Texture", 2D) = "white" {}
        _Invert ("Invert", Float) = 0
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
        ZTest Always
        Cull Off
        Blend SrcAlpha OneMinusSrcAlpha
        BlendOp Add

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            sampler2D _MainTex;
            sampler2D _RuleTex;
            float _Invert;

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
                float4 color : COLOR; // color.a = threshold / 255
            };

            struct v2f
            {
                float4 vertex : SV_POSITION;
                float2 uv : TEXCOORD0;
                float4 color : COLOR;
            };

            v2f vert(appdata v)
            {
                v2f o;
                o.vertex = v.vertex;
                o.uv = v.uv;
                o.color = v.color;
                return o;
            }

            half4 frag(v2f i) : SV_Target
            {
                half4 src = tex2D(_MainTex, i.uv);
                half4 rule = tex2D(_RuleTex, i.uv);
                float threshold = saturate(i.color.a);
                float r = rule.b;
                float mask = 1.0 - step(threshold, r);
                if (_Invert > 0.5)
                    mask = 1.0 - mask;
                src.rgb *= mask;
                src.a *= mask;
                return src;
            }
            ENDCG
        }
    }
}
