#include <functional>
#include <deque>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <emscripten/bind.h>

using namespace emscripten;

class HNode {
    public:
        HNode(emscripten::val node)
        {
            value = node["value"].as<float>();

            if(node["children"].typeof().as<std::string>() == "object") {
                std::vector<emscripten::val> children_ = emscripten::vecFromJSArray<emscripten::val>(node["children"]);

                for (int i = 0; i < children_.size(); i++)
                {
                    children.push_back(new HNode(children_[i], this));
                }
            }
        }

        HNode(emscripten::val node, HNode* parent_)
        : HNode(node)
        {
            parent = parent_;
        }

        float value = 0;
        float x = 0;
        float y = 0;
        float r = 0;

        std::vector<HNode *> children;
        HNode* parent;

        HNode eachBefore(
            std::function<void(HNode*)> callback
        ) {
            std::deque<HNode*> nodes(1, this);
            HNode* node;

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

        HNode eachAfter(
            std::function<void(HNode*)> callback
        ) {
            std::deque<HNode*> nodes = {this};
            std::deque<HNode*> next = {};
            HNode *node;

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

        // void doSomeStuff() {
        //     std::cout << parent->value << std::endl;
        // }
};

void place(HNode* a, HNode* b, HNode* c) {
    float ax = a->x;
    float ay = a->y;
    float da = b->r + c->r;
    float db = a->r + c->r;
    float dx = b->x - ax;
    float dy = b->y - ay;
    float dc = dx * dx + dy * dy;
    float db_ = db - dc; //Might need to revert this to match place

    if(dc > 0) {
        float x = 0.5 + ((db *= db) - (da *= da)) / (2 * dc);
        float y = sqrt(fmax(0, 2 * da * (db + dc) - db_ * db_ - da * da)) / (2 * dc);
        c->x = ax + x * dy + y * dy;
        c->y = ay + x * dy - y * dx;
    } else {
        c->x = ax + db;
        c->y = ay;
    }
}

struct HLNode {
    HLNode(HNode* circle)
    : _(circle)
    {}

    HNode *_;
    HLNode *next;
    HLNode *previous;
};

bool intersects(HNode* a, HNode* b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dr = a->r + b->r;
    return dr * dr - 1e-6 > dx * dx + dy * dy;
}

float distance2(HLNode* node, float x, float y) {
    HNode *a = node->_;
    HNode *b = node->next->_;
    float ab = a->r + b->r;
    float dx = (a->x * b->r + b->x * a->r) / ab - x;
    float dy = (a->y * b->r + b->y * a->r) / ab - y;
    return dx * dx + dy * dy;
}

HNode* enclose(std::vector<HNode*> nodes) {
    return nodes[0];
}

float packEnclose(std::vector<HNode *> circles)
{
    int n = circles.size();

    if(n == 0) {
        return 0;
    }

    HNode *a;
    HNode *b;
    HNode *c;

    a = circles[0];
    a->x = 0;
    a->y = 0;

    if(n == 1) {
        return a->r;
    }

    b = circles[1];
    a->x = -b->r;
    b->x = a->r;
    b->y = 0;

    if(n == 2) {
        return a->r + b->r;
    }

    c = circles[2];
    place(b, a, c);

    float aa = a->r * a->r;
    float ba = b->r * b->r;
    float ca = c->r * c->r;
    float oa = aa + ba + ca;
    float ox = aa * a->x + ba * b->x + ca * c->x;
    float oy = aa * a->y + ba * b->y + ca * c->y;
    float cx;
    float cy;
    int i;
    HLNode* j;
    HLNode* k;
    float sj;
    float sk;
    bool kill;

    HLNode *a_ = new HLNode(a);
    HLNode *b_ = new HLNode(b);
    HLNode *c_;

    for (i = 3; i < n; ++i) {
        c = circles[i];
        place(a_->_, b_->_, c);
        c_ = new HLNode(c);
        j = b_->next;
        k = a_->previous;
        sj = b_->_->r;
        sk = a_->_->r;
        kill = false;
        do
        {
            if (sj <= sk)
            {
                if(intersects(j->_, c_->_)) {
                    b_ = j;
                    a_->next = b_;
                    b_->previous = a_;
                    --i;
                    kill = true;
                } else {
                    sj += j->_->r;
                    j = j->next;
                }
            } else {
                if(intersects(k->_, c_->_)) {
                    a_ = k;
                    a_->next = b_;
                    b_->previous = a_;
                    --i;
                    kill = true;
                } else {
                    sk += k->_->r;
                    k = k->previous;
                }
            }

            if(kill) {
                break;
            }
        } while (j != k->next);

        if(kill) {
            continue;
        }

        c_->previous = a_;
        c_->next = b_;
        c_ = b_;
        a_->next = b_->previous;

        oa += ca = c_->_->r * c_->_->r;
        ox += ca * c_->_->x;
        oy += ca * c_->_->y;

        aa = distance2(a_, cx = ox / oa, cy = oy / oa);
        while((c_ = c_->next) != b_) {
            if((ca = distance2(c_, cx, cy)) < aa) {
                a_ = c_;
                aa = ca;
            }
        }
        b_ = a_->next;
    }

    std::vector<HNode *> avec = std::vector<HNode *>();
    avec.push_back(b_->_);
    c_ = b_;
    while((c_ = c_->next)!= b_) {
        avec.push_back(c_->_);
    }
    c = enclose(avec);

    for (i = 0; i < n; ++i) {
        a = circles[i];
        a->x -= c->x;
        a->y -= c->y;
    }

    return c->r;
}

std::function<void(HNode*)> radiusLeaf(
    std::function<float(HNode*)> radius){
    return [radius](HNode* node) {
        if(node->children.size() <= 0)
        {
            node->r = fmax(0, radius(node));
        }
    };
};

std::function<void(HNode *)> packChildren(
    std::function<float(HNode *)> padding,
    float k){
    return [padding, k](HNode *node) {
        if(!node->children.empty())
        {
            std::vector<HNode *> children = node->children;
            int i = 0;
            int n = children.size();
            float r = fmax(padding(node) * k, 0);
            if (r > 0)
            {
                for (i = 0; i < n; ++i) {
                    children[i]->r += r;
                }
            }
            float e = packEnclose(children);
            if (r > 0)
            {
                for (i = 0; i < n; ++i) {
                    children[i]->r -= r;
                }
            }
            node->r = e + r;
        }
    };
};

std::function<void(HNode *)> translateChild(float k) {
    return [k](HNode *node) {
        HNode *parent = node->parent;
        node->r *= k;
        if(parent != NULL) {
            node->x = parent->x + k * node->x;
            node->y = parent->y + k * node->y;
        }
    };
};

class Hierarchy {
  public:
    Hierarchy(emscripten::val options, HNode* root)
    : options(options)
    , root(root)
    {}

    bool isType(emscripten::val value, const std::string& type) {
        return (value.typeof().as<std::string>() == type);
    }

    float dx;
    float dy;
    float padding = 0;
    emscripten::val options;
    HNode *root;

    HNode* pack()
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
        std::function<float(HNode *)> radiusFunction = [this](HNode *node) {
            if(this->isType(options["radius"], "function")) {
                return options["radius"](node).as<float>();
            }
            return sqrt(node->value);
        };

        std::function<float(HNode *)> paddingFunction = [this](HNode *node) {
            if(this->isType(options["padding"], "function"))
            {
                return options["padding"](node).as<float>();
            }

            return padding;
        };

        std::function<float(HNode *)> zeroFn = [](HNode *node) { return 0; };

        bool hasRadius = this->isType(options["radius"], "function") ||
            this->isType(options["radius"], "float");

        if(hasRadius) {
            root->eachBefore(radiusLeaf(radiusFunction));
            root->eachAfter(packChildren(paddingFunction, 0.5));
            root->eachBefore(translateChild(1));
        } else {
            root->eachBefore(radiusLeaf(radiusFunction));
            root->eachAfter(packChildren(zeroFn, 1));
            root->eachAfter(packChildren(paddingFunction, root->r / fmin(dx, dy)));
            root->eachBefore(translateChild(fmin(dx, dy) / (2 * root->r)));
        }

        return root;
    };
};

Hierarchy* createHierarchy(emscripten::val options, emscripten::val node) {
    HNode *root = new HNode(node);
    return new Hierarchy(options, root);
}

EMSCRIPTEN_BINDINGS(hierarchy) {
    register_vector<HNode*>("VectorHNode");

    class_<HNode>("HNode")
        .constructor<emscripten::val>()
        // .constructor<float>()
        // .constructor<float, std::vector<HNode *>>()
        .property("children", &HNode::children)
        // .property("parent", &HNode::getParent, &HNode::setParent)
        .property("value", &HNode::value);
    // .property("x", &HNode::x)
    // .property("y", &HNode::y)
    // .property("r", &HNode::r)

    // .function("eachBefore", &HNode::eachBefore)
    // .function("eachAfter", &HNode::eachAfter);
    // .function("doSomeStuff", &HNode::doSomeStuff);

    // function("radiusLeaf", &radiusLeaf);
    // function("packChildren", &packChildren);
    // function("translateChild", &translateChild);

    class_<Hierarchy>("Hierarchy")
        .constructor(&createHierarchy, allow_raw_pointers())

        // .property("dx", &Hierarchy::dx)
        // .property("dy", &Hierarchy::dy)
        .function("pack", &Hierarchy::pack, allow_raw_pointers());
};
