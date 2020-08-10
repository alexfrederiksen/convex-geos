/*
 * A slight adaptation of Przemysław Uznański's codebase for generating convex geometries
 * by conversion from antimatroids.
 *
 * See Przemysław Uznański blog:
 * https://paracombinatorics.wordpress.com/2013/04/14/enumeration-of-antimatroids-part-iii/
 *
 */

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

typedef unsigned long long ull;
typedef unsigned int us;
typedef unsigned int uint;


int n = 3;
int N = 1<<n;
int max_irreduc_length = 0;

const int factorial[] = {1,1,2,6,24,120,720,5040};

const int max_dimension = 6;
int dimension_counts[max_dimension] = { 0 };

int one_letter = 3;
vector<int> missing_one;

// Convex geometry operations ##################################################

void print_set(uint set)
{
    for (uint j = 0; j < n; j++)
    {
        uint in_set = (set >> j) & 0x01;
        if (in_set)
        {
            cout << (char) ('a' + j) << " ";
        }
    }
}

int element_of(ull element, ull set)
{
    return (set >> element) & 0x01;
}

int subset_of(ull A, ull B)
{
    return (B | A) == B;
}

int is_comparable(ull A, ull B)
{
    ull set_union = A | B;
    return (set_union == A) || (set_union == B);
}

ull set_minus(ull A, ull B)
{
    return A &~ B;
}

// check if only one bit is set
int has_one_bit(ull x)
{
    return x && (!(x & (x - 1)));
}

int one_element_extension_of(ull A, ull B)
{
    uint difference = set_minus(A, B);
    uint is_one_elm = has_one_bit(difference);

    return subset_of(B, A) && is_one_elm;
}

int is_meet_irreduc(ull geo, uint set)
{
    // find supersets
    uint superset_count = 0;
    for (uint geo_set = 0; geo_set < N; geo_set++)
    {
        if (element_of(geo_set, geo))
        {
            // check if "geo_set" is a one-element super set of "set"
            if (one_element_extension_of(geo_set, set)) 
            {
            //    cout << " E: { ";
            //    print_set(geo_set);

            //    cout << " is an extension of ";

            //    print_set(set);
            //    cout << " } " << endl;

                superset_count++;
            }
            if (superset_count > 1) return 0;
        }
    }

    return 1;
}

// TODO: fix for all sized sets
uint set_size(ull set)
{
    int size = 0;
    for (int i = 0; i < 32; i++)
    {
        if ((set >> i) & 0x01)
            size++;
    }

    return size;
}

/* represent implications pred -> then */
struct implication_t
{
    uint pred;
    uint then;
};

int has_an_e(implication_t a)
{
    int e_set = 0x01 << one_letter;
    return subset_of(e_set, a.pred) || subset_of(e_set, a.then);
}

/* check if a is redundant by b */
int redundant_by(implication_t a, implication_t b)
{
    // if "a" contains "b" // 's predicate, and the result is just a subset
    // of "b"'s result, then "a" is useless
    return (subset_of(b.pred, a.pred)) && (subset_of(a.then, b.then));
}

void append_and_reduce_implication(vector<implication_t> & v, implication_t x)
{
    vector<implication_t>::iterator it = v.begin();
    while (it != v.end())
    {
        implication_t appended = *it;
        if (redundant_by(x, appended))
        {
            // dont append x
            return;
        } else if (redundant_by(appended, x))
        {
            // then remove appended 
            it = v.erase(it);
            continue;
        }

        ++it;
    }

    // otherwise x is not redundant and should be appended
    v.push_back(x);
}

/* compute the closure of a set in a given geometry */
uint closure_of(ull geo, uint set)
{
    int min_size = 999;
    uint min_ext = 0;

    // essentially just choose the smallest superset of "set"
    for (uint geo_set = 0; geo_set < N; geo_set++)
    {
        if (element_of(geo_set, geo))
        {
            if (subset_of(set, geo_set))
            {
                // "geo_set" is a superset of set
                int size = set_size(geo_set);
                if (size < min_size)
                {
                    min_size = size;
                    min_ext = geo_set;
                }
            }
        }
    }

    return min_ext;
}

/* naive brute force check if collection represents an antichain
 feasible_sets will consist of meet irreducibles */
bool is_antichain(vector<ull> feasible_sets, uint collection)
{
    int vec_size = feasible_sets.size();
    for (ull slow = 0; slow < (vec_size - 1); slow++)
    {
        if (!((collection >> slow) & 0x01))
        {
            continue;
        }
        for (ull fast = slow + 1; fast < vec_size; fast++)
        {
            if (!((collection >> fast) & 0x01))
            {
                continue;
            }
            if (is_comparable(feasible_sets[slow], feasible_sets[fast]))
            {
                return false;
            }
        }
    }
    return true;
}

int comparable_to(uint a, uint b)
{
    return subset_of(a, b) || subset_of(b, a);
}

// TODO: calculate the runtime of this thing lmaooooo
uint max_antichain_size(ull q)
{
    if (q == 0) return 0;

    int max = 0;

    // TODO: remove lone points for optimization (:


    for (int i = 0; i < N; i++)
    {
        if (element_of(i, q))
        {
            // choose this point
            uint selected = i;
            ull q_prime = q;

            // remove selected point
            q_prime &= ~(0x01 << i);

            // remove all points its related to
            for (int j = 0; j < N; j++)
            {
                if (element_of(j, q) &&  comparable_to(selected, j))
                {
                    // remove set j from available sets
                    q_prime &= ~(0x01 << j);
                }
            }

            // do a recursion, uwu
            int size = 1 + max_antichain_size(q_prime);

            if (size > max) max = size;
        }
    }

    return max;
}

// The big printing function ###################################################

/* converts antimatroid to convex geometry (: */
void matroid_to_convexgeo(ull matroid, int number)
{
    ull geo = 0x00;

    // list convex sets and build "geo"
    cout << "convex sets: {" << endl;
    for (uint i = 0; i < N; i++)
    {
        if (element_of(i, matroid))
        {
            // convert antimatroid set to convex geo set (take complement)
            uint convex_set = (~i & (N - 1));
            // and append to convex geometry
            geo |= (((ull) 0x01) << convex_set);

            // also print as we goooo
            cout << "  { ";
            print_set(convex_set);
            //cout << convex_set;
            cout << "}" << endl;
        }
    } 
    cout << "}" << endl;

    // also lets list all the non-convex sets
    vector<implication_t> implications;
    for (uint i = 0; i < N; i++)
    {
        if (!element_of(i, matroid)) {
            // this set is not in our antimatroid, i.e. its
            // complement is not in our convex geo

            // convert antimatroid set to convex geo set (take complement)
            uint nonconvex_set = (~i & (N - 1));
            uint closure = closure_of(geo, nonconvex_set);
            uint implication = set_minus(closure, nonconvex_set);

            append_and_reduce_implication(implications, {.pred = nonconvex_set,
                                                         .then = implication});
        }
    } 

    cout << "implications / closures: {" << endl;
    bool has_e = false;
    for (implication_t impl : implications)
    {
        has_e |= has_an_e(impl);
        cout << " *{ ";
        print_set(impl.pred);
        cout << "} -> { ";
        print_set(impl.then);
        cout << "} " << endl;
    }
    cout << "}" << endl;

    if (!has_e) missing_one.push_back(number);

    // find meet irreducible antichain
    int irreduc_length = 0;
    ull irreducibles = 0x01;

    cout << "meet-irreducibles: " << endl;
    vector<ull> meet_irrs;
    // we have seen that for |X| < 5, we will have at most 12 meet irreducibles
    int max_mis = 16;
    meet_irrs.reserve(max_mis);

    for (uint set = 0; set < N; set++)
    {
        if (element_of(set, geo) && set != 0 && set != (N - 1))
        {
            if (is_meet_irreduc(geo, set))
            {
                meet_irrs.push_back(set);
                irreduc_length++;
                irreducibles |= (((ull) 0x01) << set);
                cout << " { ";
                print_set(set);
                cout << "} " ;
            }
        }
    }
    
    cout << endl;
  
    // find the the largest antichain
    ull collection_size = ((ull) 1) << meet_irrs.size();
    ull curr_max = 0;
    uint curr_max_size = 0;
    for (ull set = 1; set < collection_size; set++)
    {
        uint sz = set_size(set);
        if (is_antichain(meet_irrs, set) && (sz > curr_max_size))
        {
            curr_max_size = sz;
            curr_max = set;
        }
    }

    cout << "cdim: " << set_size(curr_max) << endl;
    cout << "max antichain:" << endl;
    for (int i = 0; i < meet_irrs.size(); i++)
    {
        if (!((curr_max >> i) & 0x01))
        {
            continue;
        }
        cout << " { ";
        print_set(meet_irrs[i]);
        cout << "} ";
    }
    cout << endl;
    int dim = max_antichain_size(irreducibles);
    cout << "dimension: " << dim << endl;

    // count that dimension
    dimension_counts[dim - 1]++;

    cout << "id: " << geo << endl;

    // update max statistic
    max_irreduc_length = max(max_irreduc_length, irreduc_length);
}

// Beyond this is no man's land ################################################

class u128
{
    public:
        ull low;
        ull high;
        u128(ull _low=0, ull _high = 0)
        {
            low = _low;
            high = _high;
        }

        bool operator<(const u128& b) const
        {
            return (high==b.high) ? (low<b.low) : (high<b.high);
        }

        bool operator==(const u128& b) const
        {
            return (high==b.high) && (low==b.low);
        }

        u128 operator~() const
        {
            return u128(~low,~high);
        }

        u128 operator&(const u128& b) const
        {
            return u128(low&b.low, high&b.high);
        }

        u128 operator|=(const u128& b)
        {
            low |= b.low;
            high |= b.high;
            return *this;
        }
        u128 operator&=(const u128& b)
        {
            low &= b.low;
            high &= b.high;
            return *this;
        }

        void setbit(unsigned i)
        {
            if(i<64)
                low |= (1ull << i);
            else
                high |= (1ull << (i-64));
        }

        void erasebit(unsigned i)
        {
            if(i<64)
                low &= ~(1ull << i);
            else
                high &= ~(1ull << (i-64));
        }

        bool getbit(unsigned i) const
        {
            return (i<64) ? (low & (1ull << i)) : (high & (1ull << (i-64)));
        }

        void print(int number) const
        {
            matroid_to_convexgeo(low, number);
        }
};

const int magic = 2000003;
int hash_(const u128& X)
{
    int ret = (X.low%magic)+(X.high%magic);
    return ret<magic?ret:ret-magic;
}
class hashtable
{
    public:
        vector<vector<u128> > table;
        int size;
        hashtable()
        {
            table.resize(magic);
            size = 0;
        }
        void insert(const u128& X)
        {
            int h = hash_(X);
            bool ok = true;
            for(auto& i : table[h])
                if(i==X)
                {
                    ok = false;
                    break;
                }
            if(ok)
            {
                table[h].push_back(X);
                size++;
            }
        }
};


vector<vector<int> > perms;
int psize;
vector<vector<int> > ap;
//vector<int> order;
vector<u128> pred;
vector<u128> succ;

int apply(vector<int> p, int x)
{
    int y = 0;
    for(int i=0;i<n;i++)
        if(x & (1<<i))
            y |= 1<<p[i];
    return y;
}

pair<u128,vector<int> > _reduce(const u128& X)
{
    u128 R;
    vector<int> C = {0};
    vector<int> nC;
    C.reserve(psize);
    nC.reserve(psize);
    R.setbit(0);
    int i=1;
    for(int bit=0;bit<n;bit++)
    {
        int ss = C.size();
        C.resize(ss*(n-bit));
        for(int it=ss;it<ss*(n-bit);it++)
            C[it] = C[it-ss]+factorial[n-1-bit];
        for(;i<(1<<(bit+1));i++)
        {
            nC.clear();
            bool v = false;
            for(auto& t : C)
            {
                bool nv = X.getbit(ap[i][t]);
                if(nv > v)
                {
                    nC.clear();
                    v = nv;
                    R.setbit(i);
                }
                if(nv == v)
                {
                    nC.push_back(t);
                }
            }
            swap(nC,C);
        }
    }
    return make_pair(R,C);
}
u128 reduce(const u128& X)
{
    return _reduce(X).first;
}
vector<int> aut(const u128& X)
{
    return _reduce(X).second;
}


long long ans = 0;
long long ans2 = 0;

vector<u128> onestep(const vector<u128> P)            
{
    hashtable nL;
    for(auto& S : P)
    {
        vector<int> automorph = aut(S);
        ans += psize/automorph.size();
        ans2 += 1;

        cout << endl << ans2 << "." << endl;
        S.print(ans2);

        u128 mask(0,0);
        for(int I=1;I<N-1;I++)
            if(S.getbit(I))
                for(int J=1;J<I;J++)
                    if(S.getbit(J))
                    {
                        int IJ = I|J;
                        if(IJ != I && IJ != J)
                            mask.setbit(IJ);
                    }


        mask = S & ~mask;
        u128 f1,f2;
        for(int I=0;I<N;I++)
            if(S.getbit(I))
            {
                f2 |= succ[I] & f1;
                f1 |= succ[I];
            }
        f1 &= ~f2 & S;
        for(int I=0;I<N;I++)
            if(f1.getbit(I))
                mask = mask & ~pred[I];

        for(int I=1;I<N-1;I++)
            if(mask.getbit(I))
            {
                for(int a=0;a<automorph.size();a++)
                    mask.erasebit(ap[I][automorph[a]]);
                u128 nS = S;
                nS.erasebit(I);
                nL.insert(reduce(nS));
            }
    }
    vector<u128> ret;
    for(auto& v : nL.table)
        for(auto& x : v)
            ret.push_back(x);
    return ret;
}

// End of no man's land ########################################################

int main()
{
    //cout << "E: False";
    //cout << is_comparable(0b10, 0b1);
    //cout << "E: False";
    //cout << is_comparable(0b1110, 0b111);
    //cout << "E: True";
    //cout << is_comparable(0b11, 0b1);
    
    cout << "Size of base set? ";
    cin >> n;
    //n = 4;
    N = 1 << n;

    

    pred.resize(N);
    succ.resize(N);
    for(int I=0;I<N;I++)
        for(int i=0;i<n;i++)
            if(I & (1<<i))
                pred[I].setbit(I & ~(1<<i));
            else
                succ[I].setbit(I | (1<<i));
    
    vector<int> start;
    for(int i=0;i<n;i++)
        start.push_back(i);
    do
    {
        perms.push_back(start);
    }
    while(next_permutation(start.begin(),start.end()));
    psize = perms.size();
   
    ap.resize(N, vector<int>(psize));
    for(int p=0;p<psize;p++)
        for(int i=0;i<N;i++)
            ap[i][p] = apply(perms[p], i);

    u128 seed;
    for(int i=0;i<N;i++)
        seed.setbit(i);

    vector<u128> L(1,seed);

    while(!L.empty())
        L = onestep(L);

    cout << "Yay!" << endl;
    cout << ans << " total convex geometries and " << ans2 << " non-isomorphic geometries found!" << endl;
    cout << "The largest meet-irreducible set was size " << max_irreduc_length << endl;

    for (int i = 0; i < max_dimension; i++)
    {
        cout << "dimension " << (i + 1) << ": " << dimension_counts[i] << endl;
    }

    for (int i : missing_one) {
        cout << i << endl;
    }
}
