//
// Created by keega on 9/18/2021.
//

#include "sim/search/BattleScumSearcher2.h"
#include "sim/search/ExpertKnowledge.h"

#include <utility>
#include <string>
#include <memory>

using namespace sts;

std::int64_t simulationIdx = 0; // for debugging

namespace sts::search {
    thread_local search::BattleScumSearcher2 *g_debug_scum_search;
}



search::BattleScumSearcher2::BattleScumSearcher2(const BattleContext &bc, search::EvalFnc _evalFnc)
    : rootState(new BattleContext(bc)), evalFnc(std::move(_evalFnc)), randGen(bc.seed+bc.floorNum) {
}

void search::BattleScumSearcher2::search(int64_t simulations) {
    g_debug_scum_search = this;

    if (isTerminalState(*rootState)) {
        auto evaluation = evaluateEndState(*rootState);
        outcomePlayerHp = rootState->player.curHp;
        bestActionSequence = {};

        root.evaluationSum = evaluation;
        root.simulationCount = 1;
    }

    for (std::int64_t simCount = 0; simCount < simulations; ++simCount) {
        step();
    }
}

void search::BattleScumSearcher2::step() {
    searchStack = {&root};
    actionStack.clear();
    BattleContext curState;
    curState = *rootState;

    while (true) {
        auto &curNode = *searchStack.back();

        if (isTerminalState(curState)) {
            updateFromPlayout(searchStack, actionStack, curState);
            return;
        }

        const bool isLeaf = curNode.edges.empty();
        if (isLeaf) {

            ++simulationIdx;
            enumerateActionsForNode(curNode, curState);
            const auto selectIdx = selectFirstActionForLeafNode(curNode);
            auto &edgeTaken = curNode.edges[selectIdx];

//            edgeTaken.action.printDesc(std::cout, curState) << std::endl;
            edgeTaken.action.execute(curState);

            actionStack.push_back(edgeTaken.action);
            searchStack.push_back(&edgeTaken.node);

            playoutRandom(curState, actionStack);
            updateFromPlayout(searchStack, actionStack, curState);
            return;

        } else {
            const auto selectIdx = selectBestEdgeToSearch(curNode);
            auto &edgeTaken = curNode.edges[selectIdx];

//            edgeTaken.action.printDesc(std::cout, curState) << std::endl;
            edgeTaken.action.execute(curState);

            actionStack.push_back(edgeTaken.action);
            searchStack.push_back(&edgeTaken.node);
        }
    }
}

void search::BattleScumSearcher2::updateFromPlayout(const std::vector<Node *> &stack, const std::vector<Action> &actionStack, const BattleContext &endState) {
    const auto evaluation = evaluateEndState(endState);

    if (evaluation > bestActionValue) {
        bestActionSequence = actionStack;
        bestActionValue = evaluation;
        outcomePlayerHp = endState.player.curHp;
    }

    if (evaluation < minActionValue) {
        minActionValue = evaluation;
    }

    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        auto &node = *(*it);
        ++node.simulationCount;
        node.evaluationSum += evaluation;
    }
}

bool search::BattleScumSearcher2::isTerminalState(const BattleContext &bc) const { // maybe can optimize by making this evaluate directly if score cannot possibly be higher than best
    return bc.outcome != Outcome::UNDECIDED;
}

double search::BattleScumSearcher2::evaluateEdge(const search::BattleScumSearcher2::Node &parent, int edgeIdx) {

    const auto &edge = parent.edges[edgeIdx];

    double qualityValue = 0;
    if (!bestActionSequence.empty()) {
        auto avgEvaluation = edge.node.evaluationSum / (edge.node.simulationCount+1);
        double evalRange = bestActionValue - minActionValue;
        qualityValue = avgEvaluation / evalRange;
    }

    double explorationValue = explorationParameter *
            std::sqrt(std::log(parent.simulationCount+1) / (edge.node.simulationCount+1));

    return qualityValue + explorationValue;
}

int search::BattleScumSearcher2::selectBestEdgeToSearch(const search::BattleScumSearcher2::Node &cur) {
    if (cur.edges.size() == 1) {
        return 0;
    }

    auto bestEdge = 0;
    auto bestEdgeValue = evaluateEdge(cur, bestEdge);

    for (int i = 1; i < cur.edges.size(); ++i) {
        const auto value = evaluateEdge(cur, i);
        if (value > bestEdgeValue) {
            bestEdge = i;
            bestEdgeValue = value;
        }
    }
    return bestEdge;
}

int search::BattleScumSearcher2::selectFirstActionForLeafNode(const search::BattleScumSearcher2::Node &leafNode) {
    auto dist = std::uniform_int_distribution<int>(0, static_cast<int>(leafNode.edges.size())-1);
    return dist(randGen);
}

void search::BattleScumSearcher2::playoutRandom(BattleContext &state, std::vector<Action> &actionStack) {
    Node tempNode; // temp
    while (!isTerminalState(state)) {
        ++simulationIdx;
        enumerateActionsForNode(tempNode, state);
        if (tempNode.edges.empty()) {
            std::cerr << state.seed << " " << simulationIdx << std::endl;
            std::cerr << state.monsters.arr[0].getName() << " " << state.floorNum << " " << monsterEncounterStrings[static_cast<int>(state.encounter)] << std::endl;
            assert(false);
        }

        auto dist = std::uniform_int_distribution<int>(0, static_cast<int>(tempNode.edges.size())-1);
        const int selectedIdx = dist(randGen);

        const auto action = tempNode.edges[selectedIdx].action;
//        action.printDesc(std::cout, state) << std::endl;
        actionStack.push_back(action);
        action.execute(state);

        tempNode.edges.clear();
    }
}

void search::BattleScumSearcher2::enumerateActionsForNode(search::BattleScumSearcher2::Node &node,
                                                               const BattleContext &bc) {
    auto actions = Action::getAllActionsInState(bc);
    for (auto a : actions) {
        node.edges.push_back({a});
    }

#ifdef sts_print_debug
    std::cout << "{ (" << node.edges.size() << ") ";
    for (int i = 0; i < node.edges.size(); ++i) {
        node.edges[i].action.printDesc(std::cout, bc) << ", ";
    }
    std::cout << " }" << std::endl;
#endif
}

void search::BattleScumSearcher2::enumerateCardActions(search::BattleScumSearcher2::Node &node,
                                                            const BattleContext &bc) {
}

void search::BattleScumSearcher2::enumeratePotionActions(search::BattleScumSearcher2::Node &node,
                                                              const BattleContext &bc) {
}

void search::BattleScumSearcher2::enumerateCardSelectActions(search::BattleScumSearcher2::Node &node,
                                                                  const BattleContext &bc) {
}

double getNonMinionMonsterCurHpRatio(const BattleContext &bc) {
    int curHpTotal = 0;
    int maxHpTotal = 0;

    for (int i = 0; i < bc.monsters.monsterCount; ++i) {
        const auto &m = bc.monsters.arr[i];
        if (!m.hasStatus<MS::MINION>() && m.id != sts::MonsterId::INVALID) {
            curHpTotal += m.curHp;
            maxHpTotal += m.maxHp;
        }
    }

    if (curHpTotal == 0 || maxHpTotal == 0) {
        return 0;
    }

    return (double)curHpTotal / maxHpTotal;
}

double search::BattleScumSearcher2::evaluateEndState(const BattleContext &bc) {
    double potionScore = bc.potionCount * 4;

    if (bc.outcome == Outcome::PLAYER_VICTORY) {
        return 100 * (35 + bc.player.curHp + potionScore - (bc.turn * 0.01));

    } else {
//        double statusScore =
//                (bc.player.getStatus<PS::STRENGTH>() * .5);
        const bool couldHaveSpikers = bc.encounter == MonsterEncounter::THREE_SHAPES || bc.encounter == MonsterEncounter::FOUR_SHAPES;
        double energyPenalty = bc.energyWasted * -0.2 * (couldHaveSpikers ? 0 : 1);
        double drawBonus = bc.cardsDrawn * 0.03;
        double aliveScore = bc.monsters.monstersAlive*-1;

        return (1-getNonMinionMonsterCurHpRatio(bc))*10 + aliveScore + energyPenalty + drawBonus + potionScore / 2 + (bc.turn * .2);
    }
}

struct LayerStruct {
    const search::BattleScumSearcher2::Node *node;
    BattleContext *bc;
    int edgeIdx;
};

typedef std::pair<search::BattleScumSearcher2::Edge, std::unique_ptr<const BattleContext>> EdgeInfo;

std::vector<EdgeInfo> getEdgesForLayer(const search::BattleScumSearcher2 &s, int layerNum) {
    if (layerNum <= 0) {
        return {};
    }

    std::vector<EdgeInfo> layerEdges;

    std::vector<LayerStruct> curStack { {&s.root, new BattleContext(*s.rootState), 0} };

    while (!curStack.empty()) {
        if (curStack.size() == layerNum) {
            for (const auto &edge : curStack.back().node->edges) {
                layerEdges.emplace_back(edge, new BattleContext(*curStack.back().bc));
            }
        }

       // curStack size less than layerNum
       const bool visitedAll = curStack.back().edgeIdx >= curStack.back().node->edges.size();
       if (visitedAll || curStack.size() == layerNum) {
           delete curStack.back().bc;
           curStack.pop_back();
           continue;
       }

        // visit next edge
        auto &nextIdx = curStack.back().edgeIdx;
        const auto action = curStack.back().node->edges[nextIdx].action;

        BattleContext bc(*curStack.back().bc);
        action.execute(bc);

        curStack.push_back( {&curStack.back().node->edges[nextIdx++].node, new BattleContext(bc), 0} );
    }

    return layerEdges;
}

void search::BattleScumSearcher2::printSearchTree(std::ostream &os, int levels) {
    std::vector<std::vector<EdgeInfo>> layerEdges;
    for (int depth = 1; depth <= levels; ++depth) {
        layerEdges.push_back(getEdgesForLayer(*this, depth));
    }

//    auto maxIt = std::max(layerEdges.begin(), layerEdges.end(), [](auto a, auto b) { return a->size() < b->size(); });
//    if (maxIt == layerEdges.end()) {
//        return;
//    }
//    // maxIt points to something
//    const auto maxSize = maxIt->size();
//    constexpr auto edgeWidth = 30;

    for (int depth = 0; depth < levels; ++depth) {
        for (const auto &x : layerEdges[depth]) {
            os << "(" << x.first.node.simulationCount << ")";
            x.first.action.printDesc(os, *x.second) << "\t";
        }
        std::cout << '\n';
    }

}

void search::BattleScumSearcher2::printSearchStack(std::ostream &os, bool skipLast) {
    for (int i = 0; i < actionStack.size(); ++i) {
        const auto &a = actionStack[i];
        os << std::hex << a.bits << '\n';
    }

    os.flush();

//    BattleContext curBc = *rootState;
//    os << "explorationParameter: " << explorationParameter << '\n';
//    os << "bestActionValue: " << bestActionValue << '\n';
//    os << "minActionValue: " << minActionValue << '\n';
//    os << "outcomePlayerHp: " << outcomePlayerHp << '\n';
//    os << "root node:\n";
//    os << curBc << "\n";
//
//    for (int i = 0; i < actionStack.size(); ++i) {
//        if (i < searchStack.size()) {
//            const auto &n = searchStack[i];
//            os << i << " nodeSearched: " << n->simulationCount << " { ";
//            for (const auto &edge : n->edges) {
//                os << "(" << edge.node.simulationCount << ")";
//                edge.action.printDesc(os, curBc) << " ";
//            }
//            os << "}\n";
//        }
//
//        const auto &a = actionStack[i];
//        os << i << " actionTaken: ";
//        a.printDesc(os, curBc) << '\n';
//
//        if (skipLast && (i + 1 >= actionStack.size())) {
//            break;
//        }
//
//        a.execute(curBc);
//        os << curBc << '\n';
//    }
//
//    os.flush();
}
