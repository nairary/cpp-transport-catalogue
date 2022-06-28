#include <cstddef>
#include "json_builder.h"
#include "json.h"

namespace json {

    using nullptr_t = std::nullptr_t;

    Builder::Builder() {
        nodes_stack_.push_back(&root_);
    }

    KeyItemContext Builder::Key(const std::string& key) {

        if (nodes_stack_.size() == 1 || !std::holds_alternative<Dict>(*nodes_stack_.back())) {
            throw std::logic_error("Key called outside of Dictionary");
        }

        if (nodes_stack_.size() != 1 && std::holds_alternative<Dict>(*nodes_stack_[nodes_stack_.size() - 2])
            && std::holds_alternative<std::string>(*nodes_stack_.back())) {
            throw std::logic_error("Key called after previous Key was called");
        }

        if (std::holds_alternative<Dict>(*nodes_stack_.back())) {
            auto *value = new Node::Value(key);
            nodes_stack_.push_back(value);
        }

        return std::move(*this);
    }

    Builder &Builder::Value(Node::Value value) {


        if (is_finished_) {
            throw std::logic_error("Value called when object creation is finished");
        }

        if (nodes_stack_.size() > 1 && !std::holds_alternative<Array>(*nodes_stack_.back())
            && !std::holds_alternative<Dict>(*nodes_stack_[nodes_stack_.size() - 2])) {
            throw std::logic_error("Value called in unsuitable place");
        }

        if (nodes_stack_.size() != 1 && std::holds_alternative<Dict>(*nodes_stack_[nodes_stack_.size() - 2])) {
            std::visit([&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, nullptr_t>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<nullptr_t>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                } else if constexpr(std::is_same_v<T, Array>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<Array>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                } else if constexpr(std::is_same_v<T, Dict>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<Dict>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                } else if constexpr(std::is_same_v<T, bool>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<bool>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                } else if constexpr(std::is_same_v<T, int>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<int>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                } else if constexpr(std::is_same_v<T, double>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<double>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                } else if constexpr(std::is_same_v<T, std::string>) {
                    std::get<Dict>(*nodes_stack_[nodes_stack_.size() - 2]).insert(
                            {std::get<std::string>(*nodes_stack_.back()), Node(std::get<std::string>(value))});
                    delete nodes_stack_.back();
                    nodes_stack_.pop_back();
                }
            }, value);
        } else if (std::holds_alternative<Array>(*nodes_stack_.back())) {
            std::visit([&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, nullptr_t>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<nullptr_t>(value));
                } else if constexpr(std::is_same_v<T, Array>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<Array>(value));
                } else if constexpr(std::is_same_v<T, Dict>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<Dict>(value));
                } else if constexpr(std::is_same_v<T, bool>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<bool>(value));
                } else if constexpr(std::is_same_v<T, int>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<int>(value));
                } else if constexpr(std::is_same_v<T, double>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<double>(value));
                } else if constexpr(std::is_same_v<T, std::string>) {
                    std::get<Array>(*nodes_stack_.back()).emplace_back(std::get<std::string>(value));
                }
            }, value);
        } else if (nodes_stack_.size() == 1) {
            root_ = value;
            is_finished_ = true;
        }
        return *this;
    }

        DictItemContext Builder::StartDict() {

        if (is_finished_) {
            throw std::logic_error("Dictionary started when object creation is finished");
        }

        if (nodes_stack_.size() > 1 && !std::holds_alternative<Array>(*nodes_stack_.back())
            && !std::holds_alternative<Dict>(*nodes_stack_[nodes_stack_.size() - 2])) {
            throw std::logic_error("Dictionary creation in unsuitable place");
        }

        if (std::holds_alternative<Array>(*nodes_stack_.back())) {
            std::get<Array>(*nodes_stack_.back()).emplace_back(Dict());
            Node *ptr = &std::get<Array>(*nodes_stack_.back()).back();
            nodes_stack_.push_back(&const_cast<Node::Value &>(ptr->GetValue()));
        } else {
            auto *value = new Node::Value(Dict());
            nodes_stack_.push_back(value);
        }
        return std::move(*this);
    }

    ArrayItemContext Builder::StartArray() {

        if (is_finished_) {
            throw std::logic_error("Array is started when object creation is finished");
        }

        if (nodes_stack_.size() > 1 && !std::holds_alternative<Array>(*nodes_stack_.back())
            && !std::holds_alternative<Dict>(*nodes_stack_[nodes_stack_.size() - 2])) {
            throw std::logic_error("Array creation in unsuitable place");
        }

        if (std::holds_alternative<Array>(*nodes_stack_.back())) {
            std::get<Array>(*nodes_stack_.back()).emplace_back(Array());
            Node *ptr = &std::get<Array>(*nodes_stack_.back()).back();
            nodes_stack_.push_back(&const_cast<Node::Value &>(ptr->GetValue()));
        } else {
            auto *value = new Node::Value(Array());
            nodes_stack_.push_back(value);
        }
        return std::move(*this);
    }

    Builder &Builder::EndDict() {

        if (!std::holds_alternative<Dict>(*nodes_stack_.back())) {
            throw std::logic_error("Dictionary ended in unsuitable place");
        }

        if (std::holds_alternative<Dict>(*nodes_stack_.back()) && nodes_stack_.size() == 2) {
            root_ = *nodes_stack_.back();
            delete nodes_stack_.back();
            nodes_stack_.pop_back();
            is_finished_ = true;
        } else if (std::holds_alternative<Array>(*nodes_stack_[nodes_stack_.size() - 2])) {
            nodes_stack_.pop_back();
        } else if (std::holds_alternative<Dict>(*nodes_stack_.back())) {
            Dict val = std::get<Dict>(*nodes_stack_.back());
            delete nodes_stack_.back();
            nodes_stack_.pop_back();
            this->Value(val);
        }
        return *this;
    }

    Builder &Builder::EndArray() {

        if (!std::holds_alternative<Array>(*nodes_stack_.back())) {
            throw std::logic_error("Array ended in unsuitable place");
        }

        if (std::holds_alternative<Array>(*nodes_stack_.back()) && nodes_stack_.size() == 2) {
            root_ = *nodes_stack_.back();
            delete nodes_stack_.back();
            nodes_stack_.pop_back();
            is_finished_ = true;
        } else if (std::holds_alternative<Array>(*nodes_stack_[nodes_stack_.size() - 2])) {
            nodes_stack_.pop_back();
        } else if (std::holds_alternative<Array>(*nodes_stack_.back())) {
            Array val = std::get<Array>(*nodes_stack_.back());
            delete nodes_stack_.back();
            nodes_stack_.pop_back();
            this->Value(val);
        }
        return *this;
    }

    Node Builder::Build() {

        if (!is_finished_ || nodes_stack_.size() != 1) {
            throw std::logic_error("Trying to build unfinished object");
        }
        if (std::holds_alternative<nullptr_t>(root_)) {
            return (std::get<std::nullptr_t>(root_));
        } else if (std::holds_alternative<Array>(root_)) {
            return (std::get<Array>(root_));
        } else if (std::holds_alternative<Dict>(root_)) {
            return (std::get<Dict>(root_));
        } else if (std::holds_alternative<bool>(root_)) {
            return (std::get<bool>(root_));
        } else if (std::holds_alternative<int>(root_)) {
            return (std::get<int>(root_));
        } else if (std::holds_alternative<double>(root_)) {
            return (std::get<double>(root_));
        } else if (std::holds_alternative<std::string>(root_)) {
            return (std::get<std::string>(root_));
        }
        return {};
    }
    //
    DictItemContext::DictItemContext(Builder &&builder)
    : builder_(builder)
    {
    }

    KeyItemContext DictItemContext::Key(const std::string& key) {
        return std::move(builder_.Key(std::move(key)));
    }

    Builder &DictItemContext::EndDict() {
        return builder_.EndDict();
    }
    //
    ArrayItemContext::ArrayItemContext(Builder&& builder)
            : builder_(builder)
    {
    }

    ArrayItemContext& ArrayItemContext::Value(Node::Value value) {
        builder_.Value(std::move(value));
        return *this;
    }

    DictItemContext ArrayItemContext::StartDict() {
        return std::move(builder_.StartDict());
    }

    ArrayItemContext ArrayItemContext::StartArray() {
        return std::move(builder_.StartArray());
    }

    Builder& ArrayItemContext::EndArray() {
        return builder_.EndArray();
    }
    //
    KeyItemContext::KeyItemContext(Builder&& builder)
            : builder_(builder)
    {
    }

    DictItemContext KeyItemContext::Value(Node::Value value) {
        return std::move(builder_.Value(std::move(value)));
    }

    DictItemContext KeyItemContext::StartDict() {
        return std::move(builder_.StartDict());
    }

    ArrayItemContext KeyItemContext::StartArray() {
        return std::move(builder_.StartArray());
    }

}