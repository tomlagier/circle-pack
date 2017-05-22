#include <functional>
#include <vector>
#include <deque>
#include <emscripten/bind.h>

using namespace emscripten;

class MyNode {
    public:
        MyNode() {}

        MyNode(int value)
        : value(value) {}

        MyNode(std::vector<MyNode> children)
        : children(children) {}

        MyNode(int value, std::vector<MyNode> children)
        : value(value)
        , children(children)
        {}

        int value;
        std::vector<MyNode> children;

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

        MyNode doSomeStuff() {
            std::function<void(MyNode)> doThing = [](MyNode node) {
                std::cout << node.value;
                node.value = 20;
            };

            eachBefore(doThing);
            return *this;
        }
};

EMSCRIPTEN_BINDINGS(hierarchy) {
    register_vector<MyNode>("VectorMyNode");

    class_<MyNode>("MyNode")
        .constructor<>()
        .constructor<int>()
        .constructor<int, std::vector<MyNode>>()
        .property("children", &MyNode::children)
        .property("value", &MyNode::value)
        .function("eachBefore", &MyNode::eachBefore)
        .function("doSomeStuff", &MyNode::doSomeStuff);
}
