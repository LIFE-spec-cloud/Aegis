#!/usr/bin/env bash
set -e

PORT=${1:-8080}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "======================================================================"
echo "   KisanSetu (MandiFlow) - Multi-Agent Simulation Web Application     "
echo "======================================================================"
echo ""
echo "Serving KisanSetu Web UI at: http://localhost:${PORT}/web/index.html"
echo "Press Ctrl+C to stop the server."
echo ""

if command -v open >/dev/null 2>&1; then
    open "http://localhost:${PORT}/web/index.html"
elif command -v xdg-open >/dev/null 2>&1; then
    xdg-open "http://localhost:${PORT}/web/index.html"
fi

cd "${ROOT_DIR}"
python3 -m http.server "${PORT}" --bind 127.0.0.1
