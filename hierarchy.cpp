#include <functional>
#include <deque>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <emscripten/bind.h>

using namespace emscripten;

class HBasis {
    public:
      HBasis(){};
      HBasis(double x, double y, double r)
          : x(x), y(y), r(r){};

      double x = 0;
      double y = 0;
      double r = 0;
};

class HNode: public HBasis {
    public:
        HNode(emscripten::val node)
        {
            value = node["value"].as<double>();

            if(node["children"].typeof().as<std::string>() == "object") {
                std::vector<emscripten::val> children_ = emscripten::vecFromJSArray<emscripten::val>(node["children"]);

                for (int i = 0; i < children_.size(); i++)
                {
                    //std::cout << "creating new hnode during hnode constructor" << std::endl;
                    children.push_back(new HNode(children_[i], this));
                }
            }
        }

        HNode(emscripten::val node, HNode* parent_)
        : HNode(node)
        {
            parent = parent_;
        }

        double value = 0;

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
};

void place(HNode* a, HNode* b, HNode* c) {
    double ax = a->x;
    double ay = a->y;
    double da = b->r + c->r;
    double db = a->r + c->r;
    double dx = b->x - ax;
    double dy = b->y - ay;
    double dc = dx * dx + dy * dy;

    if(dc > 0) {
        double x = 0.5 + ((db *= db) - (da *= da)) / (2 * dc);
        double y = sqrt(fmax(0, 2 * da * (db + dc) - (db -= dc) * db - da * da)) / (2 * dc);
        c->x = ax + x * dx + y * dy;
        c->y = ay + x * dy - y * dx;
    } else {
        c->x = ax + db;
        c->y = ay;
    }
}

struct HLNode {
    HLNode(HNode* circle)
    : _(circle)
    {
        std::cout << "Created node: " << this << std::endl;
    }

    HLNode(HNode* circle, int refs)
    : _(circle)
    , refs(refs)
    {
        std::cout << "Created node: " << this << std::endl;
    }

    void setNext(HLNode* nextNode) {
        this->next->refs--;
        nextNode->refs++;

        std::cout << "setNext: " << nextNode << " to replace " << this->next << std::endl;

        if(this->next->refs <= 0) {
            std::cout << this << " is deleting abandoned node: " << this->next << " on setNext" << std::endl;
            delete this->next;
        }
        this->next = nextNode;
    }

    void setPrevious(HLNode* prevNode) {
        this->previous->refs--;
        prevNode->refs++;

        std::cout << "setPrevious: " << prevNode << " to replace " << this->previous << std::endl;

        if(this->previous->refs <= 0) {
            std::cout << this << " is deleting abandoned node: " << this->previous << " on setPrevious" << std::endl;
            delete this->previous;
        }
        this->previous = prevNode;
    }

    HLNode* assign(HLNode* newNode) {
        this->refs--;
        newNode->refs++;

        std::cout << "Assign: " << newNode << " to replace " << this << std::endl;

        if(this->refs <= 0) {
            std::cout << this << " is deleting abandoned node: " << this << " on assign" << std::endl;
            delete this;
        }

        return newNode;
    }

    int refs = 0;
    HNode *_;
    HLNode *next = NULL;
    HLNode *previous = NULL;
};

bool intersects(HNode* a, HNode* b) {
    double dx = b->x - a->x;
    double dy = b->y - a->y;
    double dr = a->r + b->r;
    return dr * dr - 1e-6 > dx * dx + dy * dy;
}

double distance2(const HLNode* node, double x, double y) {
    HNode *a = node->_;
    HNode *b = node->next->_;
    double ab = a->r + b->r;
    double dx = (a->x * b->r + b->x * a->r) / ab - x;
    double dy = (a->y * b->r + b->y * a->r) / ab - y;
    return dx * dx + dy * dy;
}

bool encloses(HBasis *a, HBasis* b) {
    double dr = a->r - b->r + 1e-9;
    double dx = b->x - a->x;
    double dy = b->y - a->y;
    return dr > 0 && dr * dr > dx * dx + dy * dy;
}

HBasis* encloseBasis1(HBasis* a) {
    //std::cout << "creating new basis(1)" << std::endl;
    return new HBasis(a->x, a->y, a->r);
}

HBasis* encloseBasis2(HBasis* a, HBasis* b) {
    double x1 = a->x, y1 = a->y, r1 = a->r;
    double x2 = b->x, y2 = b->y, r2 = b->r;
    double x21 = x2 - x1, y21 = y2 - y1, r21 = r2 - r1;
    double l = sqrt(x21 * x21 + y21 * y21);
    //std::cout << "creating new basis(1.2)" << std::endl;
    return new HBasis(
        (x1 + x2 + x21 / l * r21) / 2,
        (y1 + y2 + y21 / l * r21) / 2,
        (l + r1 + r2) / 2
    );
}

HBasis* encloseBasis3(HBasis* a, HBasis* b, HBasis* c) {
    double x1 = a->x, y1 = a->y, r1 = a->r,
      x2 = b->x, y2 = b->y, r2 = b->r,
      x3 = c->x, y3 = c->y, r3 = c->r,
      a2 = x1 - x2,
      a3 = x1 - x3,
      b2 = y1 - y2,
      b3 = y1 - y3,
      c2 = r2 - r1,
      c3 = r3 - r1,
      d1 = x1 * x1 + y1 * y1 - r1 * r1,
      d2 = d1 - x2 * x2 - y2 * y2 + r2 * r2,
      d3 = d1 - x3 * x3 - y3 * y3 + r3 * r3,
      ab = a3 * b2 - a2 * b3,
      xa = (b2 * d3 - b3 * d2) / (ab * 2) - x1,
      xb = (b3 * c2 - b2 * c3) / ab,
      ya = (a3 * d2 - a2 * d3) / (ab * 2) - y1,
      yb = (a2 * c3 - a3 * c2) / ab,
      A = xb * xb + yb * yb - 1,
      B = 2 * (r1 + xa * xb + ya * yb),
      C = xa * xa + ya * ya - r1 * r1,
      r = A ? (B + sqrt(B * B - 4 * A * C)) / (2 * A) : C / B,
      x = x1 + xa - xb * r,
      y = y1 + ya - yb * r;

  //std::cout << "creating new basis(1.3)" << std::endl;
  return new HBasis(
    x, y,
    fmax(fmax(
        sqrt((x1 -= x) * x1 + (y1 -= y) * y1) + r1,
        sqrt((x2 -= x) * x2 + (y2 -= y) * y2) + r2),
        sqrt((x3 -= x) * x3 + (y3 -= y) * y3) + r3)
    );
}

bool isBasis3(HBasis* a, HBasis* b, HBasis* c) {
    bool ret = !encloses(encloseBasis2(a, b), c)
      && !encloses(encloseBasis2(a, c), b)
      && !encloses(encloseBasis2(b, c), a);

    return ret;
}

HBasis* encloseBasis(std::vector<HBasis *> B){
    switch(B.size()) {
        case 1:
            return encloseBasis1(B[0]);
        case 2:
            return encloseBasis2(B[0], B[1]);
        case 3:
            return encloseBasis3(B[0], B[1], B[2]);
        default:
            return NULL;
        }
};

bool enclosesAll(HBasis* a, std::vector<HBasis*> B) {
    for (int i = 0; i < B.size(); ++i) {
        if (!encloses(a, B[i])) {
            return false;
        }
    }
  return true;
}

std::vector<HBasis *> extendBasis(std::vector<HBasis *> B, HBasis* p) {
    std::vector<HBasis *> rvec = std::vector<HBasis *>();
    int i;
    int j;

    if (enclosesAll(p, B))
    {
        rvec.push_back(p);
        return rvec;
    }

    for (i = 0; i < B.size(); ++i) {

        if (enclosesAll(encloseBasis2(B[i], p), B))
        {
            rvec.push_back(B[i]);
            rvec.push_back(p);
            return rvec;
        }
    }

    for (i = 0; i < B.size() - 1; ++i) {
        for (j = i + 1; j < B.size(); ++j) {
            if (isBasis3(B[i], B[j], p) && enclosesAll(encloseBasis3(B[i], B[j], p), B)) {
                rvec.push_back(B[i]);
                rvec.push_back(B[j]);
                rvec.push_back(p);
                return rvec;
            }
        }
    }
}

std::vector<HBasis*> findBasis(std::vector<HBasis *> L, int n, std::vector<HBasis *> B){
    HBasis *circle = encloseBasis(B);

    for (int i = 0; i < n; ++i) {
        HBasis *p = L[i];
        if (circle == NULL || !encloses(circle, p))
        {
            circle = encloseBasis(B = findBasis(L, i + 1, extendBasis(B, p)));
        }
    }

    return B;
};

HBasis* enclose(std::vector<HBasis*> circles) {
    return encloseBasis(findBasis(circles, circles.size(), std::vector<HBasis*>()));
}

void printNode(std::string label, HNode* node) {
    //std::cout << label << ": " << node->x << " " << node->y << " " << node->r << std::endl;
}

double packEnclose(std::vector<HNode *> circles)
{
    int n = circles.size();

    n = circles.size();

    if (n == 0)
    {
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

    double aa = a->r * a->r, ba = b->r * b->r, ca = c->r * c->r;
    double oa = aa + ba + ca;
    double ox = aa * a->x + ba * b->x + ca * c->x;
    double oy = aa * a->y + ba * b->y + ca * c->y;
    double cx, cy, sj, sk;
    int i;
    HLNode* j;
    HLNode* k;
    bool kill;

    //std::cout << "creating new hlnode (3)" << std::endl;
    HLNode *a_ = new HLNode(a, 3);
    HLNode *b_ = new HLNode(b, 3);
    HLNode *c_ = new HLNode(c, 3);

    a_->next = c_->previous = b_;
    b_->next = a_->previous = c_;
    c_->next = b_->previous = a_;

    for (i = 3; i < n; ++i) {
        place(a_->_, b_->_, c = circles[i]);

        std::cout << "Trying to place new circle" << std::endl;
        c_ = c_->assign(new HLNode(c));

        j = j->assign(b_->next);
        k = k->assign(a_->previous);
        sj = b_->_->r;
        sk = a_->_->r;
        kill = false;

        do
        {
            if (sj <= sk)
            {
                if(intersects(j->_, c_->_)) {
                    std::cout << "j: " << j << " intersects c: " << c_ << std::endl;
                    b_ = b_->assign(j);
                    a_->setNext(b_);
                    b_->setPrevious(a_);
                    --i;
                    kill = true;
                } else {
                    std::cout << "j to j->next" << std::endl;
                    sj += j->_->r;
                    j = j->assign(j->next);
                }
            } else {
                if(intersects(k->_, c_->_)) {
                    std::cout << "k: " << k << " intersects c: " << c_ << std::endl;
                    a_ = a_->assign(k);
                    a_->setNext(b_);
                    b_->setPrevious(a_);
                    --i;
                    kill = true;
                } else {
                    std::cout << "k to k->previous" << std::endl;
                    sk += k->_->r;
                    k = k->assign(k->previous);
                }
            }

            if(kill) {
                break;
            }
            std::cout << "Checking if j: " << j << " equals k->next: " << k->next << std::endl;
        } while (j != k->next);

        if(kill) {
            continue;
        }

        std::cout << "Success! Inserting " << c_ << " between " << a_ << " and " << b_ << std::endl;
        c_->setPrevious(a_);
        c_->setNext(b_);
        b_ = b_->assign(c_);
        b_->setPrevious(c_);
        a_->setNext(c_);
        std::cout << "Inserted! a: " << a_ << " b: " << b_ << " c: " << c_ << std::endl;

        oa += ca = c_->_->r * c_->_->r;
        ox += ca * c_->_->x;
        oy += ca * c_->_->y;

        std::cout << "Computing new closest circle" << std::endl;
        aa = distance2(a_, cx = ox / oa, cy = oy / oa);
        while((c_ = c_->assign(c_->next)) != b_) {
            if ((ca = distance2(c_, cx, cy)) < aa)
            {
                a_ = a_->assign(c_), aa = ca;
            }
        }
        b_ = b_->assign(a_->next);
        std::cout << "Outer loop complete, new closest circle pair generated. a: " << a_ << " b: " << b_ << " c: " << c_ << std::endl;
    }

    std::vector<HBasis *> avec = std::vector<HBasis *>();
    avec.push_back(b_->_);
    c_ = c_->assign(b_);
    while((c_ = c_->assign(c_->next))!= b_) {
        avec.push_back(c_->_);
    }

    HBasis *basis = enclose(avec);

    for (i = 0; i < n; ++i) {
        a = circles[i];
        a->x -= basis->x;
        a->y -= basis->y;
    }

    return basis->r;
}

std::function<void(HNode*)> radiusLeaf(
    std::function<double(HNode*)> radius){
    return [radius](HNode* node) {
        if(node->children.size() <= 0)
        {
            node->r = fmax(0, radius(node));
        }
    };
};

std::function<void(HNode *)> packChildren(
    std::function<double(HNode *)> padding,
    double k){
    return [padding, k](HNode *node) {
        if(!node->children.empty())
        {
            std::vector<HNode *> children = node->children;
            int i = 0;
            int n = children.size();
            double r = fmax(padding(node) * k, 0);
            if (r > 0)
            {
                for (i = 0; i < n; ++i) {
                    children[i]->r += r;
                }
            }
            double e = packEnclose(children);
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

std::function<void(HNode *)> translateChild(double k) {
    return [k](HNode *node) {
        HNode *parent = node->parent;

        node->r *= k;

        if (parent != NULL)
        {
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

    double dx = 1;
    double dy = 1;
    double padding = 0;
    emscripten::val options;
    HNode *root;

    HNode* pack()
    {

        if (this->isType(options["size"], "object"))
        {
            dx = options["size"][0].as<double>();
            dy = options["size"][1].as<double>();
        }

        if(this->isType(options["padding"], "number"))
        {
            padding = options["padding"].as<double>();
        }

        root->x = dx / 2;
        root->y = dy / 2;
        std::function<double(HNode *)> radiusFunction = [this](HNode *node) {
            if(this->isType(options["radius"], "function")) {
                return options["radius"](node).as<double>();
            }
            return sqrt(node->value);
        };

        std::function<double(HNode *)> paddingFunction = [this](HNode *node) {
            if(this->isType(options["padding"], "function"))
            {
                return options["padding"](node).as<double>();
            }

            return padding;
        };

        std::function<double(HNode *)> zeroFn = [](HNode *node) { return 0; };

        bool hasRadius = this->isType(options["radius"], "function") ||
            this->isType(options["radius"], "double");

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
    //std::cout << "creating new hnode (1)" << std::endl;
    HNode *root = new HNode(node);
    //std::cout << "creating new hierarchy (1)" << std::endl;
    return new Hierarchy(options, root);
}

EMSCRIPTEN_BINDINGS(hierarchy) {
    register_vector<HNode*>("VectorHNode");

    class_<HBasis>("HBasis")
        .property("x", &HBasis::x)
        .property("y", &HBasis::y)
        .property("r", &HBasis::r);

    class_<HNode, base<HBasis>>("HNode")
        .constructor<emscripten::val>()
        .property("children", &HNode::children)
        .property("value", &HNode::value);

    class_<Hierarchy>("Hierarchy")
        .constructor(&createHierarchy)
        .function("pack", &Hierarchy::pack, allow_raw_pointers());
};
