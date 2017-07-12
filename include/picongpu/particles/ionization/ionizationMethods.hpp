/* Copyright 2014-2017 Marco Garten
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 * This file contains methods needed for ionization like: particle creation functors  */

#pragma once

#include "pmacc/types.hpp"
#include "pmacc/particles/operations/Assign.hpp"
#include "picongpu/traits/attribute/GetMass.hpp"

#include "pmacc/nvidia/rng/RNG.hpp"
#include "pmacc/nvidia/rng/methods/Xor.hpp"
#include "pmacc/nvidia/rng/distributions/Uniform_float.hpp"
#include "pmacc/mpi/SeedPerRank.hpp"
#include "pmacc/traits/GetUniqueTypeId.hpp"
#include "pmacc/particles/operations/Deselect.hpp"

namespace picongpu
{
namespace particles
{
namespace ionization
{

    using namespace pmacc;

    /* operations on particles */
    namespace partOp = pmacc::particles::operations;

    /** \struct WriteElectronIntoFrame
     *
     * \brief functor that fills an electron frame entry with details about the created particle
     */
    struct WriteElectronIntoFrame
    {
        /** Functor implementation
         *
         * \tparam T_parentIon type of the particle which is ionized
         * \tparam T_childElectron type of the electron that will be created
         */
        template<typename T_parentIon, typename T_childElectron>
        DINLINE void operator()(T_parentIon& parentIon,T_childElectron& childElectron)
        {

            /* each thread sets the multiMask hard on "particle" (=1) */
            childElectron[multiMask_] = 1;
            const float_X weighting = parentIon[weighting_];

            /* each thread initializes a clone of the parent ion but leaving out
             * some attributes:
             * - multiMask: reading from global memory takes longer than just setting it again explicitly
             * - momentum: because the electron would get a higher energy because of the ion mass
             * - boundElectrons: because species other than ions or atoms do not have them
             * (gets AUTOMATICALLY deselected because electrons do not have this attribute)
             */
            auto targetElectronClone = partOp::deselect<bmpl::vector2<multiMask, momentum> >(childElectron);

            partOp::assign(targetElectronClone, partOp::deselect<particleId>(parentIon));

            const float_X massIon = attribute::getMass(weighting,parentIon);
            const float_X massElectron = attribute::getMass(weighting,childElectron);

            const float3_X electronMomentum (parentIon[momentum_]*(massElectron/massIon));

            childElectron[momentum_] = electronMomentum;

            /* conservation of momentum
             * \todo add conservation of mass */
            parentIon[momentum_] -= electronMomentum;
        }
    };

} // namespace ionization

} // namespace particles

} // namespace picongpu

