/*=========================================================================
*
*  Copyright NumFOCUS
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*         http://www.apache.org/licenses/LICENSE-2.0.txt
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*=========================================================================*/
%extend itk::simple::Transform {
   %pythoncode %{

        def __copy__(self):
          """Create a SimpleITK shallow copy, where the internal transform is shared with a copy on write implementation."""
          return self.__class__(self)

        def __deepcopy__(self, memo):
          """Create a new copy of the data and internal ITK Transform object."""
          dc = self.__class__(self)
          dc.MakeUnique()
          return dc

        def __setstate__(self, args):
          if args[0] != 0:
            raise ValueError("Unable to handle SimpleITK.Transform pickle version {0}".args[0])

          if len(args) == 1:
            return

          state = namedtuple('state_tuple_0', "version fixed_parameters parameters")(*args)

          self.SetFixedParameters(state.fixed_parameters)
          self.SetParameters(state.parameters)


        def __reduce_ex__(self, protocol ):
          version = 0

          if self.__class__ is DisplacementFieldTransform:
            args = (self.GetDisplacementField(), )
            S = (version, )
          if self.__class__ is BSplineTransform:
            args = (tuple(self.GetCoefficientImages()), self.GetOrder())
            S = (version, )
          else:
            args = (self.GetDimension(),)
            S = (version, self.GetFixedParameters(), self.GetParameters())

          return self.__class__, args, S

          %}

};
