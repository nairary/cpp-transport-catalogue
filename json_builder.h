#pragma once

#include "json.h"

#include <memory>
#include <utility>

namespace json {

    class DictItemContext;
    class ArrayItemContext;
    class KeyItemContext;

    class Builder {
    public:
        Builder();
        virtual KeyItemContext Key(const std::string&);
        Builder& Value(Node::Value);
        virtual DictItemContext StartDict();
        virtual ArrayItemContext StartArray();
        virtual Builder& EndDict();
        virtual Builder& EndArray();
        Node Build();

    private:
        Node::Value root_;
        std::vector<Node::Value*> nodes_stack_;
        bool is_finished_ = false;
    };

    class DictItemContext final : Builder {
    public:
        DictItemContext(Builder&& builder);
        Builder& EndDict() override;
        KeyItemContext Key(const std::string& key) override;

    private:
        Builder& builder_;
    };

    class ArrayItemContext final : Builder {
    public:
        ArrayItemContext(Builder&& builder);
        ArrayItemContext& Value(Node::Value value);
        DictItemContext StartDict() override;
        ArrayItemContext StartArray() override;
        Builder& EndArray() override;

    private:
        Builder& builder_;
    };

    class KeyItemContext final : Builder {
    public:
        KeyItemContext(Builder&& builder);
        DictItemContext Value(Node::Value);
        DictItemContext StartDict() override;
        ArrayItemContext StartArray() override;

    private:
      Builder& builder_;
    };

}