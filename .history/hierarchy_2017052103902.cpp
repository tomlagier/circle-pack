#include <functional>
#include <vector>
#include <deque>
#include <emscripten/bind.h>

using namespace emscripten;

class MyNode {
    public:
        MyNode() {}

        MyNode(std::vector<MyNode> children)
        : children(children) {}

        std::vector<MyNode> children;
        std::vector<MyNode> getChildren() const {
            return children;
        }

        void setChildren(std::vector<MyNode> children_) {
            children = children_;
        }

        MyNode eachBefore(
            std::function<void(MyNode)> callback
        ) {
            std::deque<MyNode> nodes(1, *this);
            MyNode node;

            do {
                node = nodes.front();
                nodes.pop_front();
                callback(node);
                if (!node.children.empty())
                {
                    for (int i = 0;  i < node.children.size(); i++) {
                        nodes.push_back(node.children[i]);
                    }
                }
            } while (!nodes.empty());

            return *this;
        }
};

EMSCRIPTEN_BINDINGS(hierarchy) {
    class_<MyNode>("MyNode")
    .constructor<>()
    .property("children", &MyNode::getChildren, &MyNode::setChildren)
    .function("eachBefore", &MyNode::eachBefore)
    ;
}