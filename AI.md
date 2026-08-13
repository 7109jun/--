# 히힝 AI 만들었어요.
>**이름은 ATCI AI에요.**
>**Qwen Gogle Gemma 같은 AI를 가져왔어요.**
## 그럼 코드만 줄게요 python으로 작성했고 ONNX를 줘야 실행되요..
```
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ATCI Qwen/Gemma ONNX preparation script.

역할:
1. Qwen/Gemma ONNX를 사용하기 위한 atci_models.onnx.json 생성
2. 필요하면 Hugging Face tokenizer 저장
3. 필요하면 optimum-cli로 ONNX export 시도

주의:
- 실제 모델 파일은 사용자가 다운로드/export해야 한다.
- Gemma는 Hugging Face 라이선스 동의가 필요할 수 있다.
- 모델이 없어도 ATCI는 Mock으로 실행된다.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path


TARGETS = [
    {
        "key": "qwen-base",
        "hf_model": "Qwen/Qwen2.5-0.5B-Instruct",
        "out": "models/qwen-base-onnx",
        "task": "text-generation",
    },
    {
        "key": "qwen-coder",
        "hf_model": "Qwen/Qwen2.5-Coder-1.5B-Instruct",
        "out": "models/qwen-coder-onnx",
        "task": "text-generation",
    },
    {
        "key": "gemma-verifier",
        "hf_model": "google/gemma-2b-it",
        "out": "models/gemma-verifier-onnx",
        "task": "text-generation",
    },
]


REGISTRY = [
    {
        "name": "Qwen Director",
        "type": "director",
        "provider": "onnx",
        "path": "./models/qwen-base-onnx",
        "tokenizer_path": "./models/qwen-base-onnx/tokenizer",
        "capabilities": ["analysis", "orchestration", "finalization"],
        "context_length": 8192,
        "quantization": "fp32",
        "status": "ready",
        "metadata": {
            "hf_model": "Qwen/Qwen2.5-0.5B-Instruct",
            "note": "Qwen base model for Director/analysis/finalization",
        },
    },
    {
        "name": "Qwen Fast Coder",
        "type": "fast_coder",
        "provider": "onnx",
        "path": "./models/qwen-coder-onnx",
        "tokenizer_path": "./models/qwen-coder-onnx/tokenizer",
        "capabilities": ["fast_coding", "drafting"],
        "context_length": 8192,
        "quantization": "fp32",
        "status": "ready",
        "metadata": {
            "hf_model": "Qwen/Qwen2.5-Coder-1.5B-Instruct",
            "note": "Qwen coder model for fast coding draft",
        },
    },
    {
        "name": "Qwen Coder",
        "type": "coder",
        "provider": "onnx",
        "path": "./models/qwen-coder-onnx",
        "tokenizer_path": "./models/qwen-coder-onnx/tokenizer",
        "capabilities": ["coding", "fixing"],
        "context_length": 8192,
        "quantization": "fp32",
        "status": "ready",
        "metadata": {
            "hf_model": "Qwen/Qwen2.5-Coder-1.5B-Instruct",
            "note": "Qwen coder model for implementation and fixing",
        },
    },
    {
        "name": "Qwen Math",
        "type": "math",
        "provider": "onnx",
        "path": "./models/qwen-base-onnx",
        "tokenizer_path": "./models/qwen-base-onnx/tokenizer",
        "capabilities": ["math"],
        "context_length": 8192,
        "quantization": "fp32",
        "status": "ready",
        "metadata": {
            "hf_model": "Qwen/Qwen2.5-0.5B-Instruct",
            "note": "Qwen base model for math reasoning",
        },
    },
    {
        "name": "Qwen Vision",
        "type": "vision",
        "provider": "mock",
        "path": "builtin://mock",
        "tokenizer_path": "",
        "capabilities": ["vision"],
        "context_length": 8192,
        "quantization": "none",
        "status": "ready",
        "metadata": {
            "note": "Vision ONNX는 multimodal processor/image pipeline가 필요하므로 MVP에서는 mock 유지"
        },
    },
    {
        "name": "Qwen General Worker",
        "type": "general",
        "provider": "onnx",
        "path": "./models/qwen-base-onnx",
        "tokenizer_path": "./models/qwen-base-onnx/tokenizer",
        "capabilities": ["general"],
        "context_length": 8192,
        "quantization": "fp32",
        "status": "ready",
        "metadata": {
            "hf_model": "Qwen/Qwen2.5-0.5B-Instruct",
            "note": "Qwen base model for general tasks",
        },
    },
    {
        "name": "Qwen Embedding",
        "type": "embedding",
        "provider": "mock",
        "path": "builtin://mock",
        "tokenizer_path": "",
        "capabilities": ["embedding"],
        "context_length": 8192,
        "quantization": "none",
        "status": "ready",
        "metadata": {
            "note": "실제 embedding ONNX 모델로 교체 가능. MVP에서는 hash embedding 사용."
        },
    },
    {
        "name": "Google Gemma",
        "type": "verifier",
        "provider": "onnx",
        "path": "./models/gemma-verifier-onnx",
        "tokenizer_path": "./models/gemma-verifier-onnx/tokenizer",
        "capabilities": ["verification"],
        "context_length": 8192,
        "quantization": "fp32",
        "status": "ready",
        "metadata": {
            "hf_model": "google/gemma-2b-it",
            "note": "Gemma verifier. Hugging Face license/access may be required.",
        },
    },
]


def write_registry(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(REGISTRY, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(f"registry written: {path}")


def run_cmd(cmd: list[str]) -> None:
    printable = " ".join(str(x) for x in cmd)
    print(f"$ {printable}")
    subprocess.run(cmd, check=True)


def check_dependencies() -> None:
    missing = []

    try:
        import transformers  # noqa: F401
    except Exception:
        missing.append("transformers")

    try:
        import onnxruntime  # noqa: F401
    except Exception:
        missing.append("onnxruntime")

    try:
        import optimum  # noqa: F401
    except Exception:
        missing.append("optimum[onnxruntime]")

    try:
        import huggingface_hub  # noqa: F401
    except Exception:
        missing.append("huggingface_hub")

    if missing:
        print("Missing dependencies.")
        print("Run:")
        print('pip install "transformers" "onnxruntime" "optimum[onnxruntime]" "huggingface_hub"')
        sys.exit(1)


def save_tokenizer(target: dict[str, str]) -> None:
    from transformers import AutoTokenizer

    out_dir = Path(target["out"]) / "tokenizer"
    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"Saving tokenizer: {target['hf_model']} -> {out_dir}")

    try:
        tokenizer = AutoTokenizer.from_pretrained(
            target["hf_model"],
            trust_remote_code=True,
        )
        tokenizer.save_pretrained(out_dir)
    except Exception as exc:
        print(f"Tokenizer download/save failed: {exc}")
        print("If this is a gated model, run `huggingface-cli login` or set HF_TOKEN.")
        raise


def export_onnx(target: dict[str, str]) -> None:
    out_dir = Path(target["out"])
    out_dir.mkdir(parents=True, exist_ok=True)

    if list(out_dir.glob("*.onnx")):
        print(f"Skip ONNX export: {out_dir} already contains *.onnx")
        return

    optimum_cli = shutil.which("optimum-cli")

    if optimum_cli:
        base_cmd = [
            optimum_cli,
            "export",
            "onnx",
            "--model",
            target["hf_model"],
            "--task",
            target["task"],
        ]
    else:
        # optimum-cli가 PATH에 없으면 module 실행을 시도한다.
        base_cmd = [
            sys.executable,
            "-m",
            "optimum.exporters.onnx",
            "--model",
            target["hf_model"],
            "--task",
            target["task"],
        ]

    try:
        run_cmd(base_cmd + [str(out_dir)])
    except subprocess.CalledProcessError as exc:
        print(f"ONNX export failed: {exc}")
        print("Manual command:")
        print(
            "optimum-cli export onnx "
            f"--model {target['hf_model']} "
            f"--task {target['task']} "
            f"{out_dir}"
        )
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare ATCI Qwen/Gemma ONNX registry and models")
    parser.add_argument(
        "--registry",
        default="atci_models.onnx.json",
        help="Output registry file path",
    )
    parser.add_argument(
        "--tokenizers",
        action="store_true",
        help="Download/save tokenizers locally",
    )
    parser.add_argument(
        "--export",
        action="store_true",
        help="Export ONNX models with optimum-cli",
    )

    args = parser.parse_args()

    registry_path = Path(args.registry)
    write_registry(registry_path)

    if args.tokenizers or args.export:
        check_dependencies()

    for target in TARGETS:
        if args.tokenizers:
            save_tokenizer(target)

        if args.export:
            export_onnx(target)

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```
