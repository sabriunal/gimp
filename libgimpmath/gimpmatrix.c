/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmatrix.c
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "gimpmath.h"


/**
 * SECTION: gimpmatrix
 * @title: GimpMatrix
 * @short_description: Utilities to set up and manipulate 3x3
 *                     transformation matrices.
 * @see_also: #GimpVector2, #GimpVector3, #GimpVector4
 *
 * When doing image manipulation you will often need 3x3
 * transformation matrices that define translation, rotation, scaling,
 * shearing and arbitrary perspective transformations using a 3x3
 * matrix. Here you'll find a set of utility functions to set up those
 * matrices and to perform basic matrix manipulations and tests.
 *
 * Each matrix class has a 2 dimensional gdouble coeff member. The
 * element for row r and column c of the matrix is coeff[r][c].
 **/


#define EPSILON 1e-6


static GimpMatrix2 * matrix2_copy                  (const GimpMatrix2 *matrix);

G_DEFINE_BOXED_TYPE (GimpMatrix2, gimp_matrix2, matrix2_copy, g_free)


/*
 * GIMP_TYPE_PARAM_MATRIX2
 */

#define GIMP_PARAM_SPEC_MATRIX2(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_MATRIX2, GimpParamSpecMatrix2))

static void   gimp_param_matrix2_class_init  (GParamSpecClass *class);
static void   gimp_param_matrix2_init        (GParamSpec      *pspec);
static void   gimp_param_matrix2_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gint   gimp_param_matrix2_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

typedef struct _GimpParamSpecMatrix2 GimpParamSpecMatrix2;

struct _GimpParamSpecMatrix2
{
  GParamSpecBoxed      parent_instance;

  GimpMatrix2          default_value;
};

/**
 * gimp_param_matrix2_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a GimpMatrix2 object
 *
 * Since: 2.4
 **/
GType
gimp_param_matrix2_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_matrix2_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecMatrix2),
        0,
        (GInstanceInitFunc) gimp_param_matrix2_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "GimpParamMatrix2",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_matrix2_class_init (GParamSpecClass *class)
{
  class->value_type        = GIMP_TYPE_MATRIX2;
  class->value_set_default = gimp_param_matrix2_set_default;
  class->values_cmp        = gimp_param_matrix2_values_cmp;
}

static void
gimp_param_matrix2_init (GParamSpec *pspec)
{
  GimpParamSpecMatrix2 *cspec = GIMP_PARAM_SPEC_MATRIX2 (pspec);

  gimp_matrix2_identity (&cspec->default_value);
}

static void
gimp_param_matrix2_set_default (GParamSpec *pspec,
                                GValue     *value)
{
  GimpParamSpecMatrix2 *cspec = GIMP_PARAM_SPEC_MATRIX2 (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gint
gimp_param_matrix2_values_cmp (GParamSpec   *pspec,
                               const GValue *value1,
                               const GValue *value2)
{
  GimpMatrix2 *matrix1;
  GimpMatrix2 *matrix2;
  gint         i, j;

  matrix1 = value1->data[0].v_pointer;
  matrix2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! matrix1)
    return matrix2 != NULL ? -1 : 0;
  else if (! matrix2)
    return matrix1 != NULL;

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
      if (matrix1->coeff[i][j] != matrix2->coeff[i][j])
        return 1;

  return 0;
}

/**
 * gimp_param_spec_matrix2:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a #GimpMatrix2 value.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
gimp_param_spec_matrix2 (const gchar       *name,
                         const gchar       *nick,
                         const gchar       *blurb,
                         const GimpMatrix2 *default_value,
                         GParamFlags        flags)
{
  GimpParamSpecMatrix2 *cspec;

  g_return_val_if_fail (default_value != NULL, NULL);

  cspec = g_param_spec_internal (GIMP_TYPE_PARAM_MATRIX2,
                                 name, nick, blurb, flags);

  cspec->default_value = *default_value;

  return G_PARAM_SPEC (cspec);
}


static GimpMatrix2 *
matrix2_copy (const GimpMatrix2 *matrix)
{
  return (GimpMatrix2 *) g_memdup2 (matrix, sizeof (GimpMatrix2));
}


/**
 * gimp_matrix2_identity:
 * @matrix: A matrix.
 *
 * Sets the matrix to the identity matrix.
 */
void
gimp_matrix2_identity (GimpMatrix2 *matrix)
{
  static const GimpMatrix2 identity = { { { 1.0, 0.0 },
                                          { 0.0, 1.0 } } };

  *matrix = identity;
}

/**
 * gimp_matrix2_mult:
 * @left: The first input matrix.
 * @right: The second input matrix which will be overwritten by the result.
 *
 * Multiplies two matrices and puts the result into the second one.
 */
void
gimp_matrix2_mult (const GimpMatrix2 *left,
                   GimpMatrix2       *right)
{
  gdouble r00 = right->coeff[0][0];
  gdouble r01 = right->coeff[0][1];

  right->coeff[0][0] = (left->coeff[0][0] * r00 +
                        left->coeff[0][1] * right->coeff[1][0]);
  right->coeff[0][1] = (left->coeff[0][0] * r01 +
                        left->coeff[0][1] * right->coeff[1][1]);
  right->coeff[1][0] = (left->coeff[1][0] * r00 +
                        left->coeff[1][1] * right->coeff[1][0]);
  right->coeff[1][1] = (left->coeff[1][0] * r01 +
                        left->coeff[1][1] * right->coeff[1][1]);
}

/**
 * gimp_matrix2_determinant:
 * @matrix: The input matrix.
 *
 * Calculates the determinant of the given matrix.
 *
 * Returns: The determinant.
 *
 * Since: 2.10.16
 */

gdouble
gimp_matrix2_determinant (const GimpMatrix2 *matrix)
{
  return matrix->coeff[0][0] * matrix->coeff[1][1] -
         matrix->coeff[0][1] * matrix->coeff[1][0];
}

/**
 * gimp_matrix2_invert:
 * @matrix: The matrix that is to be inverted.
 *
 * Inverts the given matrix.
 *
 * Since: 2.10.16
 */
void
gimp_matrix2_invert (GimpMatrix2 *matrix)
{
  gdouble det = gimp_matrix2_determinant (matrix);
  gdouble temp;

  if (fabs (det) <= EPSILON)
    return;

  temp = matrix->coeff[0][0];

  matrix->coeff[0][0]  = matrix->coeff[1][1] / det;
  matrix->coeff[0][1] /= -det;
  matrix->coeff[1][0] /= -det;
  matrix->coeff[1][1]  = temp / det;
}

/**
 * gimp_matrix2_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @newx: (out): The transformed X coordinate.
 * @newy: (out): The transformed Y coordinate.
 *
 * Transforms a point in 2D as specified by the transformation matrix.
 *
 * Since: 2.10.16
 */
void
gimp_matrix2_transform_point (const GimpMatrix2 *matrix,
                              gdouble            x,
                              gdouble            y,
                              gdouble           *newx,
                              gdouble           *newy)
{
  *newx = matrix->coeff[0][0] * x + matrix->coeff[0][1] * y;
  *newy = matrix->coeff[1][0] * x + matrix->coeff[1][1] * y;
}


static GimpMatrix3 * matrix3_copy                  (const GimpMatrix3 *matrix);

G_DEFINE_BOXED_TYPE (GimpMatrix3, gimp_matrix3, matrix3_copy, g_free)


/*
 * GIMP_TYPE_PARAM_MATRIX3
 */

#define GIMP_PARAM_SPEC_MATRIX3(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_MATRIX3, GimpParamSpecMatrix3))

static void   gimp_param_matrix3_class_init  (GParamSpecClass *class);
static void   gimp_param_matrix3_init        (GParamSpec      *pspec);
static void   gimp_param_matrix3_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gint   gimp_param_matrix3_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

typedef struct _GimpParamSpecMatrix3 GimpParamSpecMatrix3;

struct _GimpParamSpecMatrix3
{
  GParamSpecBoxed      parent_instance;

  GimpMatrix3          default_value;
};

/**
 * gimp_param_matrix3_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a GimpMatrix3 object
 *
 * Since: 2.8
 **/
GType
gimp_param_matrix3_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_matrix3_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecMatrix3),
        0,
        (GInstanceInitFunc) gimp_param_matrix3_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "GimpParamMatrix3",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_matrix3_class_init (GParamSpecClass *class)
{
  class->value_type        = GIMP_TYPE_MATRIX3;
  class->value_set_default = gimp_param_matrix3_set_default;
  class->values_cmp        = gimp_param_matrix3_values_cmp;
}

static void
gimp_param_matrix3_init (GParamSpec *pspec)
{
  GimpParamSpecMatrix3 *cspec = GIMP_PARAM_SPEC_MATRIX3 (pspec);

  gimp_matrix3_identity (&cspec->default_value);
}

static void
gimp_param_matrix3_set_default (GParamSpec *pspec,
                                GValue     *value)
{
  GimpParamSpecMatrix3 *cspec = GIMP_PARAM_SPEC_MATRIX3 (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gint
gimp_param_matrix3_values_cmp (GParamSpec   *pspec,
                               const GValue *value1,
                               const GValue *value2)
{
  GimpMatrix3 *matrix1;
  GimpMatrix3 *matrix2;
  gint         i, j;

  matrix1 = value1->data[0].v_pointer;
  matrix2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! matrix1)
    return matrix2 != NULL ? -1 : 0;
  else if (! matrix2)
    return matrix1 != NULL;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      if (matrix1->coeff[i][j] != matrix2->coeff[i][j])
        return 1;

  return 0;
}

/**
 * gimp_param_spec_matrix3:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a #GimpMatrix3 value.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.8
 **/
GParamSpec *
gimp_param_spec_matrix3 (const gchar       *name,
                         const gchar       *nick,
                         const gchar       *blurb,
                         const GimpMatrix3 *default_value,
                         GParamFlags        flags)
{
  GimpParamSpecMatrix3 *cspec;

  cspec = g_param_spec_internal (GIMP_TYPE_PARAM_MATRIX3,
                                 name, nick, blurb, flags);

  if (default_value)
    cspec->default_value = *default_value;

  return G_PARAM_SPEC (cspec);
}


static GimpMatrix3 *
matrix3_copy (const GimpMatrix3 *matrix)
{
  return (GimpMatrix3 *) g_memdup2 (matrix, sizeof (GimpMatrix3));
}


/**
 * gimp_matrix3_identity:
 * @matrix: A matrix.
 *
 * Sets the matrix to the identity matrix.
 */
void
gimp_matrix3_identity (GimpMatrix3 *matrix)
{
  static const GimpMatrix3 identity = { { { 1.0, 0.0, 0.0 },
                                          { 0.0, 1.0, 0.0 },
                                          { 0.0, 0.0, 1.0 } } };

  *matrix = identity;
}

/**
 * gimp_matrix3_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @newx: The transformed X coordinate.
 * @newy: The transformed Y coordinate.
 *
 * Transforms a point in 2D as specified by the transformation matrix.
 */
void
gimp_matrix3_transform_point (const GimpMatrix3 *matrix,
                              gdouble            x,
                              gdouble            y,
                              gdouble           *newx,
                              gdouble           *newy)
{
  gdouble  w;

  w = matrix->coeff[2][0] * x + matrix->coeff[2][1] * y + matrix->coeff[2][2];

  if (w == 0.0)
    w = 1.0;
  else
    w = 1.0/w;

  *newx = (matrix->coeff[0][0] * x +
           matrix->coeff[0][1] * y +
           matrix->coeff[0][2]) * w;
  *newy = (matrix->coeff[1][0] * x +
           matrix->coeff[1][1] * y +
           matrix->coeff[1][2]) * w;
}

/**
 * gimp_matrix3_mult:
 * @left: The first input matrix.
 * @right: The second input matrix which will be overwritten by the result.
 *
 * Multiplies two matrices and puts the result into the second one.
 */
void
gimp_matrix3_mult (const GimpMatrix3 *left,
                   GimpMatrix3       *right)
{
  gint    i;
  gdouble li0, li1, li2;
  gdouble r00 = right->coeff[0][0];
  gdouble r01 = right->coeff[0][1];
  gdouble r02 = right->coeff[0][2];
  gdouble r10 = right->coeff[1][0];
  gdouble r11 = right->coeff[1][1];
  gdouble r12 = right->coeff[1][2];
  gdouble r20 = right->coeff[2][0];
  gdouble r21 = right->coeff[2][1];
  gdouble r22 = right->coeff[2][2];

  for (i = 0; i < 3; i++)
    {
      li0 = left->coeff[i][0];
      li1 = left->coeff[i][1];
      li2 = left->coeff[i][2];
      right->coeff[i][0] = li0 * r00 + li1 * r10 + li2 * r20;
      right->coeff[i][1] = li0 * r01 + li1 * r11 + li2 * r21;
      right->coeff[i][2] = li0 * r02 + li1 * r12 + li2 * r22;
    }
}

/**
 * gimp_matrix3_translate:
 * @matrix: The matrix that is to be translated.
 * @x: Translation in X direction.
 * @y: Translation in Y direction.
 *
 * Translates the matrix by x and y.
 */
void
gimp_matrix3_translate (GimpMatrix3 *matrix,
                        gdouble      x,
                        gdouble      y)
{
  gdouble g, h, i;

  g = matrix->coeff[2][0];
  h = matrix->coeff[2][1];
  i = matrix->coeff[2][2];

  matrix->coeff[0][0] += x * g;
  matrix->coeff[0][1] += x * h;
  matrix->coeff[0][2] += x * i;
  matrix->coeff[1][0] += y * g;
  matrix->coeff[1][1] += y * h;
  matrix->coeff[1][2] += y * i;
}

/**
 * gimp_matrix3_scale:
 * @matrix: The matrix that is to be scaled.
 * @x: X scale factor.
 * @y: Y scale factor.
 *
 * Scales the matrix by x and y
 */
void
gimp_matrix3_scale (GimpMatrix3 *matrix,
                    gdouble      x,
                    gdouble      y)
{
  matrix->coeff[0][0] *= x;
  matrix->coeff[0][1] *= x;
  matrix->coeff[0][2] *= x;

  matrix->coeff[1][0] *= y;
  matrix->coeff[1][1] *= y;
  matrix->coeff[1][2] *= y;
}

/**
 * gimp_matrix3_rotate:
 * @matrix: The matrix that is to be rotated.
 * @theta: The angle of rotation (in radians).
 *
 * Rotates the matrix by theta degrees.
 */
void
gimp_matrix3_rotate (GimpMatrix3 *matrix,
                     gdouble      theta)
{
  gdouble t1, t2;
  gdouble cost, sint;

  cost = cos (theta);
  sint = sin (theta);

  t1 = matrix->coeff[0][0];
  t2 = matrix->coeff[1][0];
  matrix->coeff[0][0] = cost * t1 - sint * t2;
  matrix->coeff[1][0] = sint * t1 + cost * t2;

  t1 = matrix->coeff[0][1];
  t2 = matrix->coeff[1][1];
  matrix->coeff[0][1] = cost * t1 - sint * t2;
  matrix->coeff[1][1] = sint * t1 + cost * t2;

  t1 = matrix->coeff[0][2];
  t2 = matrix->coeff[1][2];
  matrix->coeff[0][2] = cost * t1 - sint * t2;
  matrix->coeff[1][2] = sint * t1 + cost * t2;
}

/**
 * gimp_matrix3_xshear:
 * @matrix: The matrix that is to be sheared.
 * @amount: X shear amount.
 *
 * Shears the matrix in the X direction.
 */
void
gimp_matrix3_xshear (GimpMatrix3 *matrix,
                     gdouble      amount)
{
  matrix->coeff[0][0] += amount * matrix->coeff[1][0];
  matrix->coeff[0][1] += amount * matrix->coeff[1][1];
  matrix->coeff[0][2] += amount * matrix->coeff[1][2];
}

/**
 * gimp_matrix3_yshear:
 * @matrix: The matrix that is to be sheared.
 * @amount: Y shear amount.
 *
 * Shears the matrix in the Y direction.
 */
void
gimp_matrix3_yshear (GimpMatrix3 *matrix,
                     gdouble      amount)
{
  matrix->coeff[1][0] += amount * matrix->coeff[0][0];
  matrix->coeff[1][1] += amount * matrix->coeff[0][1];
  matrix->coeff[1][2] += amount * matrix->coeff[0][2];
}

/**
 * gimp_matrix3_affine:
 * @matrix: The input matrix.
 * @a: the 'a' coefficient
 * @b: the 'b' coefficient
 * @c: the 'c' coefficient
 * @d: the 'd' coefficient
 * @e: the 'e' coefficient
 * @f: the 'f' coefficient
 *
 * Applies the affine transformation given by six values to @matrix.
 * The six values form define an affine transformation matrix as
 * illustrated below:
 *
 *  ( a c e )
 *  ( b d f )
 *  ( 0 0 1 )
 **/
void
gimp_matrix3_affine (GimpMatrix3 *matrix,
                     gdouble      a,
                     gdouble      b,
                     gdouble      c,
                     gdouble      d,
                     gdouble      e,
                     gdouble      f)
{
  GimpMatrix3 affine;

  affine.coeff[0][0] = a;
  affine.coeff[1][0] = b;
  affine.coeff[2][0] = 0.0;

  affine.coeff[0][1] = c;
  affine.coeff[1][1] = d;
  affine.coeff[2][1] = 0.0;

  affine.coeff[0][2] = e;
  affine.coeff[1][2] = f;
  affine.coeff[2][2] = 1.0;

  gimp_matrix3_mult (&affine, matrix);
}

/**
 * gimp_matrix3_determinant:
 * @matrix: The input matrix.
 *
 * Calculates the determinant of the given matrix.
 *
 * Returns: The determinant.
 */
gdouble
gimp_matrix3_determinant (const GimpMatrix3 *matrix)
{
  gdouble m01 = matrix->coeff[0][1];
  gdouble m02 = matrix->coeff[0][2];
  gdouble m11 = matrix->coeff[1][1];
  gdouble m12 = matrix->coeff[1][2];
  gdouble m21 = matrix->coeff[2][1];
  gdouble m22 = matrix->coeff[2][2];
  return  matrix->coeff[0][0] * (m11 * m22 - m12 * m21)
        - matrix->coeff[1][0] * (m01 * m22 - m02 * m21)
        + matrix->coeff[2][0] * (m01 * m12 - m02 * m11);
}

/**
 * gimp_matrix3_invert:
 * @matrix: The matrix that is to be inverted.
 *
 * Inverts the given matrix.
 */
void
gimp_matrix3_invert (GimpMatrix3 *matrix)
{
  gdouble det;
  gdouble m00 = matrix->coeff[0][0];
  gdouble m01 = matrix->coeff[0][1];
  gdouble m02 = matrix->coeff[0][2];
  gdouble m10 = matrix->coeff[1][0];
  gdouble m11 = matrix->coeff[1][1];
  gdouble m12 = matrix->coeff[1][2];
  gdouble m20 = matrix->coeff[2][0];
  gdouble m21 = matrix->coeff[2][1];
  gdouble m22 = matrix->coeff[2][2];

  /* To avoid redundant access to the coefficients, inline the determinant
   * formula.
   *
   * See: https://gitlab.gnome.org/GNOME/gimp/-/merge_requests/880#note_1727051
   */
  det =   m00 * (m11 * m22 - m12 * m21)
        - m10 * (m01 * m22 - m02 * m21)
        + m20 * (m01 * m12 - m02 * m11);

  if (det == 0.0)
    return;

  det = 1.0 / det;

  matrix->coeff[0][0] =   (m11 * m22 - m12 * m21) * det;
  matrix->coeff[1][0] = - (m10 * m22 - m12 * m20) * det;
  matrix->coeff[2][0] =   (m10 * m21 - m11 * m20) * det;
  matrix->coeff[0][1] = - (m01 * m22 - m02 * m21) * det;
  matrix->coeff[1][1] =   (m00 * m22 - m02 * m20) * det;
  matrix->coeff[2][1] = - (m00 * m21 - m01 * m20) * det;
  matrix->coeff[0][2] =   (m01 * m12 - m02 * m11) * det;
  matrix->coeff[1][2] = - (m00 * m12 - m02 * m10) * det;
  matrix->coeff[2][2] =   (m00 * m11 - m01 * m10) * det;
}


/*  functions to test for matrix properties  */

/**
 * gimp_matrix3_is_identity:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if the given matrix is the identity matrix.
 *
 * Returns: %TRUE if the matrix is the identity matrix, %FALSE otherwise
 */
gboolean
gimp_matrix3_is_identity (const GimpMatrix3 *matrix)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (i == j)
            {
              if (fabs (matrix->coeff[i][j] - 1.0) > EPSILON)
                return FALSE;
            }
          else
            {
              if (fabs (matrix->coeff[i][j]) > EPSILON)
                return FALSE;
            }
        }
    }

  return TRUE;
}

/**
 * gimp_matrix3_is_diagonal:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if the given matrix is diagonal.
 *
 * Returns: %TRUE if the matrix is diagonal, %FALSE otherwise
 */
gboolean
gimp_matrix3_is_diagonal (const GimpMatrix3 *matrix)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (i != j && fabs (matrix->coeff[i][j]) > EPSILON)
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * gimp_matrix3_is_affine:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if the given matrix defines an affine transformation.
 *
 * Returns: %TRUE if the matrix defines an affine transformation,
 *          %FALSE otherwise
 *
 * Since: 2.4
 */
gboolean
gimp_matrix3_is_affine (const GimpMatrix3 *matrix)
{
  return (fabs (matrix->coeff[2][0]) < EPSILON &&
          fabs (matrix->coeff[2][1]) < EPSILON &&
          fabs (matrix->coeff[2][2] - 1.0) < EPSILON);
}

/**
 * gimp_matrix3_is_simple:
 * @matrix: The matrix that is to be tested.
 *
 * Checks if we'll need to interpolate when applying this matrix as
 * a transformation.
 *
 * Returns: %TRUE if all entries of the upper left 2x2 matrix are
 *          either 0 or 1, %FALSE otherwise
 */
gboolean
gimp_matrix3_is_simple (const GimpMatrix3 *matrix)
{
  gdouble absm;
  gint    i, j;

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 2; j++)
        {
          absm = fabs (matrix->coeff[i][j]);
          if (absm > EPSILON && fabs (absm - 1.0) > EPSILON)
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * gimp_matrix3_equal:
 * @matrix1: The first matrix
 * @matrix2: The second matrix
 *
 * Checks if two matrices are equal.
 *
 * Returns: %TRUE the matrices are equal, %FALSE otherwise
 *
 * Since: 2.10.16
 */
gboolean
gimp_matrix3_equal (const GimpMatrix3 *matrix1,
                    const GimpMatrix3 *matrix2)
{
  gint i, j;

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (fabs (matrix1->coeff[i][j] - matrix2->coeff[i][j]) > EPSILON)
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * gimp_matrix4_identity:
 * @matrix: A matrix.
 *
 * Sets the matrix to the identity matrix.
 *
 * Since: 2.10.16
 */
void
gimp_matrix4_identity (GimpMatrix4 *matrix)
{
  gint i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        matrix->coeff[i][j] = i == j;
    }
}

/**
 * gimp_matrix4_mult:
 * @left: The first input matrix.
 * @right: The second input matrix which will be overwritten by the result.
 *
 * Multiplies two matrices and puts the result into the second one.
 *
 * Since: 2.10.16
 */
void
gimp_matrix4_mult (const GimpMatrix4 *left,
                   GimpMatrix4       *right)
{
  GimpMatrix4 result = {};
  gint        i, j;
  gdouble     li0, li1, li2, li3;

  for (i = 0; i < 4; i++)
    {
      li0 = left->coeff[i][0];
      li1 = left->coeff[i][1];
      li2 = left->coeff[i][2];
      li3 = left->coeff[i][3];
      for (j = 0; j < 4; j++)
        {
          result.coeff[i][j] =   li0 * right->coeff[0][j]
                               + li1 * right->coeff[1][j]
                               + li2 * right->coeff[2][j]
                               + li3 * right->coeff[3][j];
        }
    }

  *right = result;
}

/**
 * gimp_matrix4_to_deg:
 * @matrix:
 * @a: (out):
 * @b: (out):
 * @c: (out):
 *
 **/
void
gimp_matrix4_to_deg (const GimpMatrix4 *matrix,
                     gdouble           *a,
                     gdouble           *b,
                     gdouble           *c)
{
  *a = 180 * (asin (matrix->coeff[1][0]) / G_PI_2);
  *b = 180 * (asin (matrix->coeff[2][0]) / G_PI_2);
  *c = 180 * (asin (matrix->coeff[2][1]) / G_PI_2);
}

/**
 * gimp_matrix4_transform_point:
 * @matrix: The transformation matrix.
 * @x: The source X coordinate.
 * @y: The source Y coordinate.
 * @z: The source Z coordinate.
 * @newx: (out): The transformed X coordinate.
 * @newy: (out): The transformed Y coordinate.
 * @newz: (out): The transformed Z coordinate.
 *
 * Transforms a point in 3D as specified by the transformation matrix.
 *
 * Returns: The transformed W coordinate.
 *
 * Since: 2.10.16
 */
gdouble
gimp_matrix4_transform_point (const GimpMatrix4 *matrix,
                              gdouble            x,
                              gdouble            y,
                              gdouble            z,
                              gdouble           *newx,
                              gdouble           *newy,
                              gdouble           *newz)
{
  gdouble neww;

  *newx = matrix->coeff[0][0] * x +
          matrix->coeff[0][1] * y +
          matrix->coeff[0][2] * z +
          matrix->coeff[0][3];
  *newy = matrix->coeff[1][0] * x +
          matrix->coeff[1][1] * y +
          matrix->coeff[1][2] * z +
          matrix->coeff[1][3];
  *newz = matrix->coeff[2][0] * x +
          matrix->coeff[2][1] * y +
          matrix->coeff[2][2] * z +
          matrix->coeff[2][3];
  neww  = matrix->coeff[3][0] * x +
          matrix->coeff[3][1] * y +
          matrix->coeff[3][2] * z +
          matrix->coeff[3][3];

  *newx /= neww;
  *newy /= neww;
  *newz /= neww;

  return neww;
}
