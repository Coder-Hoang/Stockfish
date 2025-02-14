/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2024 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "../misc.h"
#include "../position.h"
#include "../types.h"
#include "nnue_architecture.h"
#include "nnue_feature_transformer.h"
#include "nnue_misc.h"

namespace Stockfish::Eval::NNUE {

struct EmbeddedNNUE {
    EmbeddedNNUE(const unsigned char* embeddedData,
                 const unsigned char* embeddedEnd,
                 const unsigned int   embeddedSize) :
        data(embeddedData),
        end(embeddedEnd),
        size(embeddedSize) {}
    const unsigned char* data;
    const unsigned char* end;
    const unsigned int   size;
};

extern const EmbeddedNNUE embeddedNNUEBig;
extern const EmbeddedNNUE embeddedNNUESmall;

template<typename Arch, typename Transformer>
class Network {
   public:
    Network(EvalFile file, EmbeddedNNUE embeddedEval) :
        evalFile(file),
        embedded(embeddedEval) {}

    void load(const std::string& rootDirectory, std::string evalfilePath);
    bool save(const std::optional<std::string>& filename) const;


    Value evaluate(const Position& pos,
                   bool            adjusted   = false,
                   int*            complexity = nullptr,
                   bool            psqtOnly   = false) const;


    void hint_common_access(const Position& pos, bool psqtOnl) const;

    void          verify(std::string evalfilePath) const;
    NnueEvalTrace trace_evaluate(const Position& pos) const;

   private:
    void load_user_net(const std::string&, const std::string&);
    void load_internal();

    void initialize();

    bool                       save(std::ostream&, const std::string&, const std::string&) const;
    std::optional<std::string> load(std::istream&);

    bool read_header(std::istream&, std::uint32_t*, std::string*) const;
    bool write_header(std::ostream&, std::uint32_t, const std::string&) const;

    bool read_parameters(std::istream&, std::string&) const;
    bool write_parameters(std::ostream&, const std::string&) const;

    // Input feature converter
    LargePagePtr<Transformer> featureTransformer;

    // Evaluation function
    AlignedPtr<Arch> network[LayerStacks];

    EvalFile     evalFile;
    EmbeddedNNUE embedded;

    // Hash value of evaluation function structure
    static constexpr std::uint32_t hash = Transformer::get_hash_value() ^ Arch::get_hash_value();
};

// Definitions of the network types
using SmallFeatureTransformer =
  FeatureTransformer<TransformedFeatureDimensionsSmall, &StateInfo::accumulatorSmall>;
using SmallNetworkArchitecture =
  NetworkArchitecture<TransformedFeatureDimensionsSmall, L2Small, L3Small>;

using BigFeatureTransformer =
  FeatureTransformer<TransformedFeatureDimensionsBig, &StateInfo::accumulatorBig>;
using BigNetworkArchitecture = NetworkArchitecture<TransformedFeatureDimensionsBig, L2Big, L3Big>;

using NetworkBig   = Network<BigNetworkArchitecture, BigFeatureTransformer>;
using NetworkSmall = Network<SmallNetworkArchitecture, SmallFeatureTransformer>;


struct Networks {
    Networks(NetworkBig&& nB, NetworkSmall&& nS) :
        big(std::move(nB)),
        small(std::move(nS)) {}

    NetworkBig   big;
    NetworkSmall small;
};


}  // namespace Stockfish

#endif
