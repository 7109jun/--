// JdfAll.cs — 자체 JDF v1: 파서 + 바이너리 봉인 + 서명 체인
#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;

namespace OurJdf;

// ── 1. 모델 ──
public abstract record JdfValue
{
    public sealed record Str(string Text) : JdfValue;
    public sealed record Num(double Value) : JdfValue;
    public sealed record Keyword(string Word) : JdfValue;
    public sealed record Ref(string Id) : JdfValue;
    public sealed record Emph(string Word) : JdfValue;
    public sealed record Named(string Key, JdfValue Value) : JdfValue;
}
public sealed record JdfNode(string Name, List<JdfValue> Args, List<JdfNode> Children, int Line)
{
    public bool IsLeaf => Children.Count == 0;
}

// ── 2. 파서 ──
public sealed class JdfParseException : Exception
{
    public JdfParseException(int line, string msg, string hint)
        : base($"❌ [줄 {line}] {msg}\n   💡 힌트: {hint}") { }
}

public sealed class JdfParser
{
    private const int MaxDepth = 100;
    private string _src = "";
    private int _pos, _line = 1;

    public JdfNode Parse(string source)
    {
        _src = source; _pos = 0; _line = 1;
        SkipWsAndComments();
        if (Peek() != '<')
            throw Err("문서는 <오브젝트>로 시작해야 합니다.", "예: <document>(version: \"1.0\")");
        var root = ParseNode(0);
        SkipWsAndComments();
        if (_pos < _src.Length) throw Err("문서 종료 뒤 여분 내용.", "한 파일에 <document> 하나만.");
        if (root.Name != "document") throw Err($"최상위가 <{root.Name}>.", "최상위는 <document>.");
        return root;
    }

    private JdfNode ParseNode(int depth)
    {
        if (depth > MaxDepth) throw Err($"중
