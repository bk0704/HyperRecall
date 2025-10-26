# HyperRecall Shell Aliases
# Source this file to add convenient shortcuts: source aliases.sh

# Main aliases for quick access
alias hr='./run.sh'                    # Quick run
alias hr-build='make build'            # Build only
alias hr-clean='make clean'            # Clean build
alias hr-dev='./dev.sh'                # Development build
alias hr-run='./build/bin/hyperrecall' # Run without rebuild

# Development aliases
alias hr-logs='tail -f ~/.local/share/HyperRecall/logs/*.log 2>/dev/null || echo "No logs found"'
alias hr-data='cd ~/.local/share/HyperRecall/ && pwd'
alias hr-config='cd ~/.local/share/HyperRecall/ && ls -lah'

# Build aliases
alias hr-configure='make configure'
alias hr-rebuild='make clean && make build'

echo "✅ HyperRecall aliases loaded!"
echo "Quick commands:"
echo "  hr          - Run HyperRecall"
echo "  hr-build    - Build only"
echo "  hr-dev      - Development build"
echo "  hr-clean    - Clean build"
echo "  hr-run      - Run without rebuilding"
echo ""
echo "Type 'alias | grep hr-' to see all aliases"
