# 五子棋 AI 混合算法 | A mixed algorithm for Gobang AI

该项目源自本人在电子科技大学计算机学院 2021 级程序设计基础 A 班的作业。

这个算法基于五元组估值算法和蒙特卡洛树搜索。具体地，它将原版 MCTS 中的 Simulation 阶段替换为经典的五元组估值。

更加具体地，在对一个节点进行扩展时，该算法不进行随机走子，而是利用 Minimax 中的估价方式对其所有子节点进行估值，并返回分数的最大值。然后，在选择最值得访问节点这一步骤中，该算法将 UCT 公式的左半边修改为 “所有子节点的最大分数”与胜利分数之商。

然而这样将会带来一个问题：当一个节点被扩展后，它的得分将必然下降，然而它实际上并没有变得比它的兄弟节点更劣。因此，我们需要对下棋者双方使用不同的评分表进行评分。对于下最后一步棋的一方，其评分表将会比它的对手分数更低。

由于并未经过精细的优化，本程序仅作为这种混合算法的简单参考。当然，如果你愿意，你也可以把这个半成品直接拿来玩。

This is originally my homework from "Foundation of programming design" Class A, SCSE, UESTC.

This algorithm is based on Monte Carlo Tree Search and quintuple valuation algorithm. Specifically, it replaces the Simulation step in the original MCTS with the classic quintuple valuation.

More specifically, in Expansion step, it doesn't do the rollout, instead, it evaluates all of its child nodes using the evaluation method in Minimax, and return the max of the scores. Then, in Selection step, the left side of the UCT formula is replaced as the quotient of "the max score of its child nodes" and the score of winning.

But this method brings about a problem: when a node is expanded, its score will inevitably decrease, but it doesn't mean that it becomes a worse selection than it's brother nodes. So, we need to use different score table for each player. For the one who has just made a move, his/her/its score table should contain lower scores than his/her/its rival.

Because of the lack of further optimization, this project only act as a simple reference of this kind of algorithm. And of course, there's nothing preventing you from downloading and playing with the executable if you wish.
