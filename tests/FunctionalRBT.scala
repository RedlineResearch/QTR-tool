// algorithm based on C. Okasaki, Purely Functional Data Structures, section 3.3
sealed trait Color
case object Red extends Color
case object Black extends Color

sealed abstract class Tree[+A <% Ordered[A]]
final case object E extends Tree[Nothing]
final case class T[A <% Ordered[A]](color: Color, left: Tree[A], elem: A, right: Tree[A]) extends Tree[A]


object FunctionalRBT {
  def member[A <% Ordered[A]](x: A, t: Tree[A])(implicit or: Ordering[A]): Boolean = t match {
    case E => false
    case T(_, l, v, r) =>
      if (x < v) {
        member(x, l)
      } else if (x > v) {
        member(x, r)
      } else {
        true
      }
  }

  def insert[A <% Ordered[A]](x: A, t: Tree[A])(implicit or: Ordering[A]): Tree[A] = {
    def ins(s: Tree[A]): Tree[A] = s match {
      case E => T(Red, E, x, E)
      case T(color, l, v, r) =>
        if (x < v) {
          balance(color, ins(l), v, r)
        } else if (x > v) {
          balance(color, l, v, ins(r))
        } else {
          s
        }
    }

    val (l, v, r) = ins(t) match {
      case T(_, l, v, r) => (l, v, r)
    }

    T(Black, l, v, r)
  }

  def balance[A <% Ordered[A]](c: Color, l: Tree[A], v: A, r: Tree[A])
      (implicit or: Ordering[A]): Tree[A] = (c, l, v, r) match {
    case (Black, T(Red, T(Red, a, x, b), y, c), z, d) => T(Red, T(Black, a, x, b), y, T(Black, c, z, d))
    case (Black, T(Red, a, x, T(Red, b, y, c)), z, d) => T(Red, T(Black, a, x, b), y, T(Black, c, z, d))
    case (Black, a, x, T(Red, T(Red, b, y, c), z, d)) => T(Red, T(Black, a, x, b), y, T(Black, c, z, d))
    case (Black, a, x, T(Red, b, y, T(Red, c, z, d))) => T(Red, T(Black, a, x, b), y, T(Black, c, z, d))
    case (c, l, v, r) => T(c, l, v, r)
  }

  def inOrder[A <% Ordered[A]](t: Tree[A]): Unit = t match {
    case E =>;
    case T(_, l, v, r) => {
      inOrder(l)
      println(v)
      inOrder(r)
    }
  }

  def height[A <% Ordered[A]](t: Tree[A]): Int = t match {
    case E => 0
    case T(_, l, _, r) => {
      1 + math.max(height(l), height(r))
    }
  }

  def main(args: Array[String]): Unit = {
    import scala.language.implicitConversions
    import scala.language.postfixOps
    val nums = 1 to 10000 toList
    val rbt = nums.foldLeft(E.asInstanceOf[Tree[Int]]) { (t, x) => insert(x, t) }
    println("Height of tree: " + height(rbt))
    inOrder(rbt)
  }
}
