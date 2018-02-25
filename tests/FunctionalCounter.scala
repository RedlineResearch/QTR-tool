object FunctionalCounter {
  def generateCounter(n: Int): () => Int = {
    var x = n
    () => {
      x = x + 1
      x
    }
  }

  def main(args: Array[String]): Unit = {
    val counter = generateCounter(5)
    println(counter())
    println(counter())
    println(counter())
    println(counter())
    println(counter())
    println(counter())
  }
}
