#!/usr/bin/env bash
# =============================================================================
# Automated Test Runner for Dining Philosophers Deadlock Prevention System
# Runs all 10 POSIX invariant tests with safety timeouts
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "====================================================================="
echo "   DINING PHILOSOPHERS DEADLOCK PREVENTION - TEST RUNNER             "
echo "====================================================================="
echo "Project Directory: $PROJECT_ROOT"
echo "Building test binaries..."

cd "$PROJECT_ROOT"
make test

echo "====================================================================="
echo "Automated test execution completed successfully."
echo "====================================================================="
