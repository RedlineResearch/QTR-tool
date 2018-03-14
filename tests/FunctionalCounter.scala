object FunctionalCounter {
  def generateCounter(n: Int): () => Int = {
    var x = n
    () => {
      x = x + 1
      x
    }
  }

  def main(args: Array[String]): Unit = {
    val counter = generateCounter(0)
    0 to 1000 foreach {
      _ => println(counter())
    }
  }
}
