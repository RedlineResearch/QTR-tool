import java.util.Random;

public class BinarySearchTree<T extends Comparable<T>>
{
    private T val;
    private BinarySearchTree<T> left;
    private BinarySearchTree<T> right;

    public BinarySearchTree() {
        val = null;
        left = null;
        right = null;
    }

    public BinarySearchTree(T initVal) {
        this();
        val = initVal;
    }

    public void insert(T insertVal) {
        if (val == null) {
            val = insertVal;
            return;
        }

        if (val.compareTo(insertVal) <= 0) {
            if (right == null) {
                right = new BinarySearchTree<T>(insertVal);
            } else {
                right.insert(insertVal);
            }
        } else if (left == null) {
            left = new BinarySearchTree<T>(insertVal);
        } else {
            left.insert(insertVal);
        }
    }
    
    public int height() {
        if (val == null) {
            return 0;
        } else if (left == null && right == null) {
            return 1;
        } else if (left == null) {
            return 1 + right.height();
        } else if (right == null) {
            return 1 + left.height();
        } else {
            return 1 + Math.max(left.height(), right.height());
        }
    }

    public void inOrder() {
        if (left != null) {
            left.inOrder();
        }

        if (val != null) {
            System.out.println(val);
        }
        
        if (right != null) {
            right.inOrder();
        }
    }

    public static void main(String[] args) {
        BinarySearchTree<Integer> t1 = new BinarySearchTree<>();
        BinarySearchTree<Integer> t2 = new BinarySearchTree<>();
        
        for (int i = 0; i < 1000; i++) {
            t1.insert(i);
        }

        Random rand = new Random();
        
        for (int i = 0; i < 1000; i++) {
            t2.insert(rand.nextInt(10000));
        }

        System.out.println("Height of tree 1 = " + t1.height());
        t1.inOrder();

        System.out.println("Height of tree 2 = " + t2.height());
        t2.inOrder();
    }       
}
