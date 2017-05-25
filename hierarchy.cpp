#include <functional>
#include <deque>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <emscripten/bind.h>

using namespace emscripten;

class HierarchyNode {
    public:
        HierarchyNode() {}

        HierarchyNode(float value)
        : value(value) {}

        HierarchyNode(std::vector<HierarchyNode*> children)
        : children(children) {}

        HierarchyNode(float value, std::vector<HierarchyNode*> children)
        : value(value)
        , children(children)
        {}

        float value = 0;
        float x = 0;
        float y = 0;
        float r = 0;
        std::vector<HierarchyNode *> children;

        HierarchyNode eachBefore(
            std::function<void(HierarchyNode*)> callback
        ) {
            std::deque<HierarchyNode*> nodes(1, this);
            HierarchyNode* node;

            do {
                node = nodes.front();
                nodes.pop_front();
                callback(node);
                if (!node->children.empty())
                {
                    for (int i = 0;  i < node->children.size(); i++) {
                        nodes.push_back(node->children[i]);
                    }
                }
            } while (!nodes.empty());

            return *this;
        };

        HierarchyNode eachAfter(
            std::function<void(HierarchyNode*)> callback
        ) {
            std::deque<HierarchyNode*> nodes = {this};
            std::deque<HierarchyNode*> next = {};
            HierarchyNode *node;

            do {
                node = nodes.front();
                next.push_back(node);
                nodes.pop_front();
                if (!node->children.empty())
                {
                    for (int i = 0;  i < node->children.size(); i++) {
                        nodes.push_back(node->children[i]);
                    }
                }
            } while (!nodes.empty());

            do {
                node = next.front();
                next.pop_front();
                callback(node);
            } while (!next.empty());

            return *this;
        };
};

std::function<void(HierarchyNode*)> radiusLeaf(
    std::function<float(HierarchyNode*)> radius){
    return [radius](HierarchyNode* node) {
        if(node->children.size() <= 0)
        {
            node->r = fmax(0, radius(node));
        }
    };
};

std::function<void(HierarchyNode *)> packChildren(
    std::function<float(HierarchyNode *)> padding,
    float k){
    return [padding, k](HierarchyNode *node) {
        if(!node->children.empty())
        {
            std::vector<HierarchyNode *> children = node->children;
            int i = 0;
            int n = children.size();
            float r = fmax(padding(node) * k, 0);
            if (r > 0)
            {
                for (i = 0; i < n; ++i) {
                    children[i]->r += r;
                }
            }
            int e = 0; //packEnclose(children)
            if(r > 0) {
                for (i = 0; i < n; ++i) {
                    children[i]->r -= r;
                }
            }
            node->r = e + r;
        }
    };
};

class Hierarchy {
  public:
    Hierarchy(emscripten::val options_)
    : options(options_)
    {}

    bool isType(emscripten::val value, const std::string& type) {
        //std::cout << value.typeof().as<std::string>() + "\n";
        return (value.typeof().as<std::string>() == type);
    }

    float dx;
    float dy;
    float padding;
    emscripten::val options;

    Hierarchy pack(HierarchyNode* root)
    {
        if (this->isType(options["size"], "object"))
        {
            dx = options["size"][0].as<float>();
            dy = options["size"][1].as<float>();
        }

        if(this->isType(options["padding"], "number"))
        {
            padding = options["padding"].as<float>();
        }

        root->x = dx / 2;
        root->y = dy / 2;
        std::function<float(HierarchyNode *)> radiusFunction = [this](HierarchyNode *node) {
            if(this->isType(options["radius"], "function")) {
                return options["radius"](node).as<float>();
            }
            return sqrt(node->value);
        };

        std::function<float(HierarchyNode *)> paddingFunction = [this](HierarchyNode *node) {
            if(this->isType(options["padding"], "function"))
            {
                return options["padding"](node).as<float>();
            }

            return padding;
        };

        // if(this->hasRadius) {
        root->eachBefore(radiusLeaf(radiusFunction));
        root->eachAfter(packChildren(paddingFunction, 0.5));
        // ->eachBefore(translateChild(1));
        // }

        return *this;
    };
};

EMSCRIPTEN_BINDINGS(hierarchy) {
    register_vector<HierarchyNode*>("VectorHierarchyNode");

    class_<HierarchyNode>("HierarchyNode")
        .constructor<>()
        .constructor<float>()
        .constructor<float, std::vector<HierarchyNode *>>()

        .property("children", &HierarchyNode::children)
        .property("value", &HierarchyNode::value)
        .property("x", &HierarchyNode::x)
        .property("y", &HierarchyNode::y)
        .property("r", &HierarchyNode::r)

        .function("eachBefore", &HierarchyNode::eachBefore)
        .function("eachAfter", &HierarchyNode::eachAfter);
    // .function("doSomeStuff", &HierarchyNode::doSomeStuff);

    function("radiusLeaf", &radiusLeaf);
    function("packChildren", &packChildren);

    class_<Hierarchy>("Hierarchy")
        .constructor<emscripten::val>()

        .property("dx", &Hierarchy::dx)
        .property("dy", &Hierarchy::dy)
        .function("pack", &Hierarchy::pack, allow_raw_pointers());
};
