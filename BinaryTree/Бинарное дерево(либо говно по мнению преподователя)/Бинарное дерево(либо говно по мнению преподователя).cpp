#include <iostream>
using namespace std;

// Определение структуры для узла дерева
struct Node {
	int key;
	Node* left;
	Node* right;

	Node(int k) : key(k), left(nullptr), right(nullptr) {}
};

class BinaryTree {
public:
	BinaryTree() : root(nullptr) {}

	// Метод для вставки ключа в дерево
	void insert(int key) {
		root = insertRecursive(root, key);
	}

	// Метод для поиска ключа в дереве
	bool search(int key) {
		return searchRecursive(root, key);
	}

	// Метод для удаления ключа из дерева
	void remove(int key) {
		root = removeRecursive(root, key);
	}

	// Метод для обхода дерева в порядке "in-order"
	void inOrder() {
		inOrderRecursive(root);
	}

private:
	Node* root;

	// Рекурсивный метод для вставки ключа в дерево
	Node* insertRecursive(Node* node, int key) {
		if (node == nullptr) {
			return new Node(key);
		}

		if (key < node->key) {
			node->left = insertRecursive(node->left, key);
		}
		else if (key > node->key) {
			node->right = insertRecursive(node->right, key);
		}

		return node;
	}

	// Рекурсивный метод для поиска ключа в дереве
	bool searchRecursive(Node* node, int key) {
		if (node == nullptr) {
			return false;
		}

		if (key == node->key) {
			return true;
		}
		else if (key < node->key) {
			return searchRecursive(node->left, key);
		}
		else {
			return searchRecursive(node->right, key);
		}
	}

	// Рекурсивный метод для удаления ключа из дерева
	Node* removeRecursive(Node* node, int key) {
		if (node == nullptr) {
			return node;
		}

		if (key < node->key) {
			node->left = removeRecursive(node->left, key);
		}
		else if (key > node->key) {
			node->right = removeRecursive(node->right, key);
		}
		else {
			if (node->left == nullptr) {
				Node* temp = node->right;
				delete node;
				return temp;
			}
			else if (node->right == nullptr) {
				Node* temp = node->left;
				delete node;
				return temp;
			}

			Node* temp = minValueNode(node->right);
			node->key = temp->key;
			node->right = removeRecursive(node->right, temp->key);
		}

		return node;
	}

	Node* minValueNode(Node* node) {
		Node* current = node;
		while (current->left != nullptr) {
			current = current->left;
		}
		return current;
	}

	// Рекурсивный метод для обхода дерева в порядке "in-order"
	void inOrderRecursive(Node* node) {
		if (node != nullptr) {
			inOrderRecursive(node->left);
			cout << node->key << " ";
			inOrderRecursive(node->right);
		}
	}
};

int main() {
	setlocale(LC_ALL, "Russian");

	BinaryTree tree;
	tree.insert(50);
	tree.insert(30);
	tree.insert(70);
	tree.insert(20);
	tree.insert(40);

	cout << "In-order обход дерева: ";
	tree.inOrder();
	cout << endl;

	cout << "Поиск ключа 40: " << (tree.search(40) ? "Найдено" : "Не найдено") << endl;
	cout << "Поиск ключа 60: " << (tree.search(60) ? "Найдено" : "Не найдено") << endl;

	tree.remove(30);
	cout << "In-order обход дерева после удаления 30: ";
	tree.inOrder();
	cout << endl;

	return 0;
}