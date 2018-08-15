/*
 * Increment a key
 */
typedef struct
{
   int i_0 ;
   int i_1 ;
   short s_0 ;
} db_key_t ;

/*
 * First parameter is a pointer to the key as returned from one
 * PAC Data Tables read call
 *
 * Second parameter is where the new key should be placed after incrementing,
 * in preparation for a subsequent call to PAC Data Tables.
 */
void increment_key ( const db_key_t * k0 , db_key_t * k1 )
{
   int a ;
   int b ;
   int c ;

   a = k0 -> s_0 ;
   b = k0 -> i_1 ;
   c = k0 -> i_0 ;

   /*
    * Operate the 'carry' which occurs with overflow
    */
   if (-1 == a)
   {
      if (-1 == b)
      {
         c += 1 ;
      }
      b += 1 ;
   }

   k0 -> s_0 = a+1 ;
   k0 -> i_1 = b ;
   k0 -> i_0 = c ;
}
