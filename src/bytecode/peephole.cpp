#include "generate.hpp"

namespace ogm::bytecode
{
    // indicates we can't optimize before this.
    void peephole_block(std::ostream& out, GenerateContextArgs& context_args)
    {
        context_args.m_peephole_horizon = out.tellp();
    }

    // optimize back to the last
    void peephole_optimize(std::ostream& out, GenerateContextArgs& context_args)
    {
        // TODO
    }
}