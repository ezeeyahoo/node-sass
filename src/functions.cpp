#include "functions.hpp"
#include "ast.hpp"
#include "context.hpp"
#include "parser.hpp"
#include "constants.hpp"

#include <cmath>

#define ARG(argname, argtype) get_arg<argtype>(argname, env, sig, path, line)
#define BUILT_IN(name) Expression*\
name(Env& env, Context& ctx, Signature sig, const string& path, size_t line)

namespace Sass {

  Definition* make_native_function(Signature sig, Native_Function f, Context& ctx)
  {
    Parser sig_parser = Parser::from_c_str(sig, ctx, "[built-in function]", 0);
    sig_parser.lex<Prelexer::identifier>();
    string name(sig_parser.lexed);
    Parameters* params = sig_parser.parse_parameters();
    return new (ctx.mem) Definition("[built-in function]",
                                    0,
                                    sig,
                                    name,
                                    params,
                                    f);
  }

  namespace Functions {

    template <typename T>
    T* get_arg(const string& argname, Env& env, Signature sig, const string& path, size_t line)
    {
      // Minimal error handling -- the expectation is that built-ins will be written correctly!
      T* val = dynamic_cast<T*>(env[argname]);
      string msg("argument ");
      msg += argname;
      msg += " of ";
      msg += sig;
      msg += " must be a ";
      msg += T::type_name();
      if (!val) error(msg, path, line);
      return val;
    }

    ////////////////
    // RGB FUNCTIONS
    ////////////////

    Signature rgb_sig = "rgb($red, $green, $blue)";
    BUILT_IN(rgb)
    {
      return new (ctx.mem) Color(path,
                                 line,
                                 ARG("$red",   Number)->value(),
                                 ARG("$green", Number)->value(),
                                 ARG("$blue",  Number)->value());
    }

    Signature rgba_4_sig = "rgba($red, $green, $blue, $alpha)";
    BUILT_IN(rgba_4)
    {
      return new (ctx.mem) Color(path,
                                 line,
                                 ARG("$red",   Number)->value(),
                                 ARG("$green", Number)->value(),
                                 ARG("$blue",  Number)->value(),
                                 ARG("$alpha", Number)->value());
    }

    Signature rgba_2_sig = "rgba($color, $alpha)";
    BUILT_IN(rgba_2)
    {
      Color* c_arg = ARG("$color", Color);
      Color* new_c = new (ctx.mem) Color(*c_arg);
      new_c->a(ARG("$alpha", Number)->value());
      return new_c;
    }

    Signature red_sig = "red($color)";
    BUILT_IN(red)
    { return new (ctx.mem) Number(path, line, ARG("$color", Color)->r()); }

    Signature green_sig = "green($color)";
    BUILT_IN(green)
    { return new (ctx.mem) Number(path, line, ARG("$color", Color)->g()); }

    Signature blue_sig = "blue($color)";
    BUILT_IN(blue)
    { return new (ctx.mem) Number(path, line, ARG("$color", Color)->b()); }

    Signature mix_sig = "mix($color-1, $color-2, $weight: 50%)";
    BUILT_IN(mix)
    {
      Color*  color1 = ARG("$color-1", Color);
      Color*  color2 = ARG("$color-2", Color);
      Number* weight = ARG("$weight", Number);

      double p = weight->value()/100;
      double w = 2*p - 1;
      double a = color1->a() - color2->a();

      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;

      return new (ctx.mem) Color(path,
                                 line,
                                 std::floor(w1*color1->r() + w2*color2->r()),
                                 std::floor(w1*color1->g() + w2*color2->g()),
                                 std::floor(w1*color1->b() + w2*color2->b()),
                                 color1->a()*p + color2->a()*(1-p));
    }

    ////////////////
    // HSL FUNCTIONS
    ////////////////

    // RGB to HSL helper function
    struct HSL { double h; double s; double l; };
    HSL rgb_to_hsl(double r, double g, double b)
    {
      r /= 255.0; g /= 255.0; b /= 255.0;

      double max = std::max(r, std::max(g, b));
      double min = std::min(r, std::min(g, b));
      double del = max - min;

      double h = 0, s = 0, l = (max + min)/2;

      if (max == min) {
        h = s = 0; // achromatic
      }
      else {
        if (l < 0.5) s = del / (max + min);
        else         s = del / (2.0 - max - min);

        double dr = (((max - r)/6.0) + (del/2.0))/del;
        double dg = (((max - g)/6.0) + (del/2.0))/del;
        double db = (((max - b)/6.0) + (del/2.0))/del;

        if      (r == max) h = db - dg;
        else if (g == max) h = (1.0/3.0) + dr - db;
        else if (b == max) h = (2.0/3.0) + dg - dr;

        if      (h < 0) h += 1;
        else if (h > 1) h -= 1;
      }
      HSL hsl_struct;
      hsl_struct.h = static_cast<int>(h*360)%360;
      hsl_struct.s = s*100;
      hsl_struct.l = l*100;
      return hsl_struct;
    }

  }
}