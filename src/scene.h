class SceneGraph {
public:
    NodeHandle createNode(const std::string& name = "");
    void deleteNode(NodeHandle handle);
    
    void setParent(NodeHandle child, NodeHandle parent);
    void setLocalTransform(NodeHandle node, const Transform& transform);
    
    const Transform& getWorldTransform(NodeHandle node);
    std::vector<NodeHandle> getChildren(NodeHandle node);

private:
    struct Node {
        std::string name;
        Transform localTransform;
        Transform worldTransform;
        NodeHandle parent;
        std::vector<NodeHandle> children;
    };
    
    std::unordered_map<NodeHandle, Node> nodes;
    void updateWorldTransform(NodeHandle node);
};