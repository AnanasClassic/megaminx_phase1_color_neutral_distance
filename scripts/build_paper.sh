#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ -f "$ROOT/paper/main.tex" ]]; then
  mkdir -p "$ROOT/release"
  cd "$ROOT/paper"
  for _ in 1 2 3 4; do
    pdflatex -interaction=nonstopmode -halt-on-error main.tex
  done
  if grep -Eq 'LaTeX Warning: There were undefined references|Overfull \\hbox|Overfull \\vbox' main.log; then
    echo "LaTeX produced unresolved references or overfull boxes" >&2
    exit 1
  fi
  cp main.pdf "$ROOT/release/megaminx_phase1_preprint.pdf"
  echo "Built $ROOT/release/megaminx_phase1_preprint.pdf"
elif [[ -f "$ROOT/../main.tex" ]]; then
  cd "$ROOT/.."
  for _ in 1 2 3 4; do
    pdflatex -interaction=nonstopmode -halt-on-error main.tex
  done
  if grep -Eq 'LaTeX Warning: There were undefined references|Overfull \\hbox|Overfull \\vbox' main.log; then
    echo "LaTeX produced unresolved references or overfull boxes" >&2
    exit 1
  fi
  echo "Built $ROOT/../main.pdf"
else
  echo "Could not locate main.tex" >&2
  exit 1
fi
