#pragma once

#include <string>
#include <sstream>

#include "data/Bake.hpp"
#include "iterator/Iterator3.hpp"

// Ambedo
// n.
// a kind of melancholic trance
// in which you become completely absorbed
// in vivid sensory details—raindrops skittering down a window,
// tall trees leaning in the wind,
// clouds of cream swirling in your coffee—
// briefly soaking in the experience of being alive,
// an act that is done purely for its own sake.

template<typename T>
class Cake
{
Bake bake;

public:

Cake(const Plot3 dims)
: bake(sizeof(T), dims)
{
}

Cake()
: bake()
{
}

Cake(const Bake& p_bake)
: bake(p_bake)
{
}

using Row = std::initializer_list<T>;
using Layer = std::initializer_list<Row>;

int3 plotofRows(const std::initializer_list<Row>& rows)
{
    unsigned width = 0;

    for(const auto& row: rows)
        width = std::max(width, row.size());

    return int3{width, rows.size(), 1};
}

int3 plotofLayer(const std::initializer_list<Layer>& layers)
{
    unsigned depth = layers.size();
    unsigned height = 0;
    unsigned width = 0;

    for(const auto& layer: layers)
    {
        height = std::max(height, layer.size());

        for(const auto& row: layer)
            width = std::max(width, row.size());
    }

    return int3{width, height, depth};
}

Cake(std::initializer_list<Layer> layers)
: bake(sizeof(T)
, Plot3(plotofLayer(layers)))
{
    fill(T());

    int z = 0;
    for(const auto& layer: layers)
    {
        int y = 0;
        for(const auto& row: layer)
        {
            int x = 0;
            for(const auto& cell: row)
                Cake::operator[](int3{x++,y,z}) = cell;
            y++;
        }
        z++;
    }
}

Cake(std::initializer_list<Row> rows)
: bake(sizeof(T)
, Plot3(plotofRows(rows)))
{
    fill(T());

    int y = 0;
    for(const auto& row: rows)
    {
        int x = 0;
        for(const auto& cell: row)
            operator[](int3{x++,y,0}) = cell;
        y++;
    }
}

Cake(std::initializer_list<T> cells)
: bake(sizeof(T)
, Plot3(int3{cells.size(), 1, 1}))
{
 fill(T());

 int x = 0;
 for(const auto& cell: cells)
   operator[](int3{x++, 0, 0}) = cell;
}

Cake(const Cake& cake)
: bake(cake.bake)
{
}

Cake(Cake&& movie)
{
 Bake::swap(bake, movie.bake);
}

Cake& operator=(const Cake<T>& cake)
{
 bake = cake.bake;
}

Cake& operator=(Cake&& movie)
{
 Bake::swap(bake, movie.bake);
}

T& operator[](int i)
{
 return *static_cast<T*>(bake[i]);
}

T& operator[](const int3 xyz)
{
 return *static_cast<T*>(bake[xyz]);
}

const T& operator[](const int3 xyz)const
{
 return *static_cast<const T*>(bake[xyz]);
}

T& operator[](const int2 xy)
{
 return *static_cast<T*>(bake[int3{xy[0], xy[1], 0}]);
}

const T& operator[](const int2 xy)const
{
 return *static_cast<const T*>(bake[int3{xy[0], xy[1], 0}]);
}

Cake<T>& setOffset(const int3 xyz)
{
 bake.setOffset(xyz);

 return *this;
}

const Plot3 getPlot()const
{
    return (Plot3)bake;
}

bool operator==(const Cake& rhs)const
{
 return bake == rhs.bake;
}

const Cake<T>
operator[](const Plot3 slice)const
{
 auto off3 = operator Plot3().intersection(Plot3(slice));

 auto dims = off3.dimensions;
 const int3 offs = off3.offset;

 Cake<T> cake(off3);

 for(int x = 0; x < dims[0]; x++)
 for(int y = 0; y < dims[1]; y++)
 for(int z = 0; z < dims[2]; z++)
 {
  const int3 xyz{x,y,z};

  cake[xyz] = operator[](xyz + offs);
 }

 return cake;
}

const Bake& getBake()const
{
 return bake;
}

/*void resize(const Plot3 newVol)
{

Plot3 oldVol(*this);
auto oldDims = oldVol.getDimensions();
auto newDims = newVol.getDimensions();

//dont resize if unnecessary
if(oldDims != newDims)
{
  auto newData = new T[newVol.size()];

  auto intVol = oldVol.intersection(newVol);

  auto intDims = intVol.getDimensions();
  auto intOffs = intVol.getOffset();

  for(int x = 0; x < intDims[0]; x++)
  for(int y = 0; y < intDims[1]; y++)
  for(int z = 0; z < intDims[2]; z++)
  {
    int3 xyz = int3{x,y,z}
	         + intOffs;

    newData[newVol.getIndex(xyz)] =
      operator[](xyz);
  }

  //new grid prepared,
  delete[] mData;
  mData = newData;
}

*this = newVol;
}*/


/*
const T& get(const int3 xyz,
             const T& defaulte)const
{
	return contains(xyz)
	     ? operator[](xyz)
		 : defaulte;
}

const T& operator[](int i)const
{
 return *cake[i];
}

T& operator[](const int3 xyz)
{
 return *cake[xyz];
}

const T& operator[](const int3 xyz)const
{
 return *cake[xyz];
}
*/
Cake<T>&
fill(const T& input)
{
   for(int i = 0,
	       n = size();
           i < n;
           i++)
   {
      operator[](i) = input;
   }

   return *this;
}

T* begin()
{
 return bake.begin();
}

T* end()
{
 return bake.end();
}

const T* const begin()const
{
 return static_cast<const T* const>(bake.begin());
}

const T* const end()const
{
 return static_cast<const T* const>(bake.end());
}

unsigned size()const
{
 return bake.size();
}

explicit operator Plot3()const
{
 return (Plot3)bake;
}

bool contains(const int3 xyz)const
{
 return Plot3(bake).contains(xyz);
}

operator std::string()const
{
  const auto offs = getPlot().offset;
  const auto dims = getPlot().dimensions;

  std::stringstream out;

  out << "{ offs: "
      << offs
	  << ", dims: "
	  << dims
	  << "}";

  for(int z = 0; z < dims[2]; z++){
    out << "\nz: " << z;

    for(int y = 0; y < dims[1]; y++){
	  out << "\n";

      for(int x = 0; x < dims[0]; x++){
	    out << operator[](int3{x,y,z} + offs) << ", ";
      }
    }
  }

  return out.str();
}

bool isEqual(const int3 first,
             const int3 second = int3{})const
{
	return operator[](first)
	    == operator[](second);
}

static Cake<T> build(const T* read, const int3 dimensions)
{
    const Plot3 plot(dimensions);
    Cake<T> result(plot);

    for(const auto xyz: plot)
    {
        result[xyz] = *read++;
    }

    return result;
}

};



template<typename TYPE>
std::istream&
operator>>(std::istream& in,
		   Cake<TYPE>& data)
{
	int2 dims;

	/*in >> dims;

	data = dims;

	for (auto& elem: data)
	{
		in >> elem;
	}
*/
	return in;
}

template<typename TYPE>
std::ostream&
operator<<(std::ostream& out,
		   const Cake<TYPE>& data)
{
	out << Plot3(data).dimensions[0]
		<< ' '
		<< Plot3(data).dimensions[1]
		<< ' '
		<< Plot3(data).dimensions[2]
		<< ' ';

	for (const auto& elem: data)
	{
		out << elem << "\n";
	}

	out << "}";

	return out;
}
