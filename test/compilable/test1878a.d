void main()
{
  ubyte from, to;
  foreach(i; from..to)
  {
    static assert(is(typeof(i) == ubyte));
  }
  foreach(i; 'a'..'l')
  {
    static assert(is(typeof(i) == char));
  }
}
