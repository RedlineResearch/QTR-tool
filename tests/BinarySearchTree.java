public class BinarySearchTree<T extends Comparable<T>>
{
    private T val;
    private BinarySearchTree<T> left;
    private BinarySearchTree<T> right;

    public BinarySearchTree(T rootVal) {
        this.val = rootVal;
        this.left = null;
        this.right = null;
    }

    public void insert(T insertVal) {
        if (val.compareTo(insertVal) >= 0) {
            if (left == null) {
                left = new BinarySearchTree<>(insertVal);
            } else {
                left.insert(insertVal);
            }
        } else {
            if (right == null) {
                right = new BinarySearchTree<>(insertVal);
            } else {
                right.insert(insertVal);
            }
        }
    }

    public void inOrder() {
        if (left != null) {
            left.inOrder();
        }

        System.err.println(val);

        if (right != null) {
            right.inOrder();
        }
    }

    public static void main(String[] args) {
        BinarySearchTree<Integer> tr = new BinarySearchTree<>(3);
        tr.insert(5);
        tr.insert(2);
        tr.insert(1);
        tr.insert(7);
        tr.insert(9);
        tr.inOrder();
    }
    
    
}
