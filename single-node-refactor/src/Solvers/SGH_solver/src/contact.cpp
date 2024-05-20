#include "contact.h"

// Definition of static member variables
size_t contact_patch_t::num_nodes_in_patch;
double contact_patches_t::bucket_size;
size_t contact_patches_t::num_contact_nodes;

/// beginning of global, linear algebra functions //////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
KOKKOS_FUNCTION
void mat_mul(const ViewCArrayKokkos<double> &A, const ViewCArrayKokkos<double> &x, ViewCArrayKokkos<double> &b)
{
    size_t x_ord = x.order();
    size_t m = A.dims(0);
    size_t n = A.dims(1);

    if (x_ord == 1)
    {
        for (size_t i = 0; i < m; i++)
        {
            b(i) = 0.0;
            for (size_t k = 0; k < n; k++)
            {
                b(i) += A(i, k)*x(k);
            }
        }
    } else
    {
        size_t p = x.dims(1);
        for (size_t i = 0; i < m; i++)
        {
            for (size_t j = 0; j < p; j++)
            {
                b(i, j) = 0.0;
                for (size_t k = 0; k < n; k++)
                {
                    b(i, j) += A(i, k)*x(k, j);
                }
            }
        }
    }
}  // end mat_mul
#pragma clang diagnostic pop

KOKKOS_FUNCTION
double norm(const ViewCArrayKokkos<double> &x)
{
    double sum = 0.0;
    for (size_t i = 0; i < x.size(); i++)
    {
        sum += pow(x(i), 2);
    }
    return sqrt(sum);
}  // end norm

KOKKOS_INLINE_FUNCTION
double det(const ViewCArrayKokkos<double> &A)
{
    return A(0, 0)*(A(1, 1)*A(2, 2) - A(1, 2)*A(2, 1)) - A(0, 1)*(A(1, 0)*A(2, 2) - A(1, 2)*A(2, 0)) +
           A(0, 2)*(A(1, 0)*A(2, 1) - A(1, 1)*A(2, 0));
}  // end det

KOKKOS_FUNCTION
void inv(const ViewCArrayKokkos<double> &A, ViewCArrayKokkos<double> &A_inv, const double &A_det)
{
    // A_inv = 1/det(A)*adj(A)
    A_inv(0, 0) = (A(1, 1)*A(2, 2) - A(1, 2)*A(2, 1))/A_det;
    A_inv(0, 1) = (A(0, 2)*A(2, 1) - A(0, 1)*A(2, 2))/A_det;
    A_inv(0, 2) = (A(0, 1)*A(1, 2) - A(0, 2)*A(1, 1))/A_det;
    A_inv(1, 0) = (A(1, 2)*A(2, 0) - A(1, 0)*A(2, 2))/A_det;
    A_inv(1, 1) = (A(0, 0)*A(2, 2) - A(0, 2)*A(2, 0))/A_det;
    A_inv(1, 2) = (A(0, 2)*A(1, 0) - A(0, 0)*A(1, 2))/A_det;
    A_inv(2, 0) = (A(1, 0)*A(2, 1) - A(1, 1)*A(2, 0))/A_det;
    A_inv(2, 1) = (A(0, 1)*A(2, 0) - A(0, 0)*A(2, 1))/A_det;
    A_inv(2, 2) = (A(0, 0)*A(1, 1) - A(0, 1)*A(1, 0))/A_det;
}  // end inv

KOKKOS_FUNCTION
double dot(const ViewCArrayKokkos<double> &a, const ViewCArrayKokkos<double> &b)
{
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); i++)
    {
        sum += a(i)*b(i);
    }
    return sum;
}  // end dot

KOKKOS_FUNCTION
void outer(const ViewCArrayKokkos<double> &a, const ViewCArrayKokkos<double> &b, ViewCArrayKokkos<double> &c)
{
    for (size_t i = 0; i < a.size(); i++)
    {
        for (size_t j = 0; j < b.size(); j++)
        {
            c(i, j) = a(i)*b(j);
        }
    }
}  // end outer

KOKKOS_FUNCTION
bool all(const ViewCArrayKokkos<bool> &a, const size_t &size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (!a(i))
        {
            return false;
        }
    }
    return true;
}  // end all

KOKKOS_FUNCTION
bool any(const ViewCArrayKokkos<bool> &a, const size_t &size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (a(i))
        {
            return true;
        }
    }
    return false;
}  // end any
/// end of global, linear algebra functions ////////////////////////////////////////////////////////////////////////////

/// beginning of contact_patch_t member functions //////////////////////////////////////////////////////////////////////
KOKKOS_FUNCTION  // is called in macros
void contact_patch_t::update_nodes(const mesh_t &mesh, const node_t &nodes, // NOLINT(*-make-member-function-const)
                                   const corner_t &corner)
{
    // Constructing the points, vel_points, and internal_force arrays
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < num_nodes_in_patch; j++)
        {
            const size_t node_gid = nodes_gid(j);

            points(i, j) = nodes.coords(0, node_gid, i);
            vel_points(i, j) = nodes.vel(0, node_gid, i);
            internal_force(i, j) = 0.0;

            // looping over the corners
            for (size_t corner_lid = 0; corner_lid < mesh.num_corners_in_node(node_gid); corner_lid++)
            {
                size_t corner_gid = mesh.corners_in_node(node_gid, corner_lid);
                internal_force(i, j) += corner.force(corner_gid, i);
            }

            // construct the mass
            mass_points(i, j) = nodes.mass(node_gid);

            // construct the acceleration
            acc_points(i, j) = internal_force(i, j)/mass_points(i, j);

        }  // end local node loop
    }  // end dimension loop
}  // end update_nodes

void contact_patch_t::capture_box(const double &vx_max, const double &vy_max, const double &vz_max,
                                  const double &ax_max, const double &ay_max, const double &az_max,
                                  const double &dt)
{
    // collecting all the points that will be used to construct the bounding box into add_sub
    // the bounding box is the maximum and minimum points for each dimension
    // todo: add_sub needs to be moved to a member variable. Can't have this being allocated for every call.
    CArrayKokkos<double> add_sub(2, 3, contact_patch_t::num_nodes_in_patch);
    FOR_ALL_CLASS(i, 0, contact_patch_t::num_nodes_in_patch, {
        add_sub(0, 0, i) = points(0, i) + vx_max*dt + 0.5*ax_max*dt*dt;
        add_sub(0, 1, i) = points(1, i) + vy_max*dt + 0.5*ay_max*dt*dt;
        add_sub(0, 2, i) = points(2, i) + vz_max*dt + 0.5*az_max*dt*dt;
        add_sub(1, 0, i) = points(0, i) - vx_max*dt - 0.5*ax_max*dt*dt;
        add_sub(1, 1, i) = points(1, i) - vy_max*dt - 0.5*ay_max*dt*dt;
        add_sub(1, 2, i) = points(2, i) - vz_max*dt - 0.5*az_max*dt*dt;
    });
    Kokkos::fence();

    // todo: Does bounds(i) = result_max and bounds(i + 3) = result_min need to be inside Run()?
    for (int i = 0; i < 3; i++)
    {
        // Find the max of dim i
        double local_max;
        double result_max;
        REDUCE_MAX(j, 0, contact_patch_t::num_nodes_in_patch, local_max, {
            if (local_max < add_sub(0, i, j))
            {
                local_max = add_sub(0, i, j);
            }
        }, result_max);
        bounds(i) = result_max;

        // Find the min of dim i
        double local_min;
        double result_min;
        REDUCE_MIN(j, 0, contact_patch_t::num_nodes_in_patch, local_min, {
            if (local_min > add_sub(1, i, j))
            {
                local_min = add_sub(1, i, j);
            }
        }, result_min);
        bounds(i + 3) = result_min;
    }
    Kokkos::fence();
}  // end capture_box

KOKKOS_FUNCTION
void contact_patch_t::construct_basis(ViewCArrayKokkos<double> &A, const double &del_t) const
{
//    double local_acc_arr[3*contact_patch_t::max_nodes];
//    ViewCArrayKokkos<double> local_acc(&local_acc_arr[0], 3, num_nodes_in_patch);
//    for (int i = 0; i < 3; i++)
//    {
//        for (int j = 0; j < num_nodes_in_patch; j++)
//        {
//            const contact_node_t &node_obj = nodes_obj(j);
//            local_acc(i, j) = (node_obj.contact_force(i) + node_obj.internal_force(i))/node_obj.mass;
//        }
//    }

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < num_nodes_in_patch; j++)
        {
            A(i, j) = points(i, j) + vel_points(i, j)*del_t + 0.5*acc_points(i, j)*del_t*del_t;
        }
    }
}  // end construct_basis

KOKKOS_FUNCTION  // will be called inside a macro
bool contact_patch_t::get_contact_point(const contact_node_t &node, double &xi_val, double &eta_val,
                                        double &del_tc) const
{
    // In order to understand this, just see this PDF:
    // https://github.com/gabemorris12/contact_surfaces/blob/master/Finding%20the%20Contact%20Point.pdf

    // The python version of this is also found in contact.py from that same repo.

    // Using "max_nodes" for the array size to ensure that the array is large enough
    double A_arr[3*contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> A(&A_arr[0], 3, num_nodes_in_patch);

    double phi_k_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> phi_k(&phi_k_arr[0], num_nodes_in_patch);

    double d_phi_d_xi_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_phi_d_xi_(&d_phi_d_xi_arr[0], num_nodes_in_patch);

    double d_phi_d_eta_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_phi_d_eta_(&d_phi_d_eta_arr[0], num_nodes_in_patch);

    double d_A_d_del_t_arr[3*contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_A_d_del_t(&d_A_d_del_t_arr[0], 3, num_nodes_in_patch);

    double rhs_arr[3];  // right hand side (A_arr*phi_k)
    ViewCArrayKokkos<double> rhs(&rhs_arr[0], 3);

    double lhs;  // left hand side (node.pos + node.vel*del_t + 0.5*node.acc*del_t*del_t)

    double F_arr[3];  // defined as rhs - lhs
    ViewCArrayKokkos<double> F(&F_arr[0], 3);

    double J0_arr[3];  // column 1 of jacobian
    ViewCArrayKokkos<double> J0(&J0_arr[0], 3);

    double J1_arr[3];  // column 2 of jacobian
    ViewCArrayKokkos<double> J1(&J1_arr[0], 3);

    double J2_arr[3];  // column 3 of jacobian
    ViewCArrayKokkos<double> J2(&J2_arr[0], 3);

    double J_arr[9];  // jacobian
    ViewCArrayKokkos<double> J(&J_arr[0], 3, 3);

    double J_inv_arr[9];  // inverse of jacobian
    ViewCArrayKokkos<double> J_inv(&J_inv_arr[0], 3, 3);

    double J_det;  // determinant of jacobian
    double sol[3];  // solution containing (xi, eta, del_tc)
    sol[0] = xi_val;
    sol[1] = eta_val;
    sol[2] = del_tc;

    double grad_arr[3];  // J_inv*F term
    ViewCArrayKokkos<double> grad(&grad_arr[0], 3);

    // begin Newton-Rasphson solver
    size_t iters;  // need to keep track of the number of iterations outside the scope of the loop
    for (int i = 0; i < max_iter; i++)
    {
        iters = i;
        construct_basis(A, del_tc);
        phi(phi_k, xi_val, eta_val);
        mat_mul(A, phi_k, rhs);
        for (int j = 0; j < 3; j++)
        {
            lhs = node.pos(j) + node.vel(j)*(del_tc) + 0.5*node.acc(j)*(del_tc)*(del_tc);
            F(j) = rhs(j) - lhs;
        }

        if (norm(F) <= tol)
        {
            break;
        }

        d_phi_d_xi(d_phi_d_xi_, xi_val, eta_val);
        d_phi_d_eta(d_phi_d_eta_, xi_val, eta_val);

        // Construct d_A_d_del_t
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < num_nodes_in_patch; k++)
            {
                d_A_d_del_t(j, k) = vel_points(j, k) + acc_points(j, k)*(del_tc);
            }
        }

        mat_mul(A, d_phi_d_xi_, J0);
        mat_mul(A, d_phi_d_eta_, J1);
        mat_mul(d_A_d_del_t, phi_k, J2);
        // lhs is a function of del_t, so have to subtract the derivative of lhs wrt del_t
        for (int j = 0; j < 3; j++)
        {
            J2(j) = J2(j) - node.vel(j) - node.acc(j)*(del_tc);
        }

        // Construct the Jacobian
        for (int j = 0; j < 3; j++)
        {
            J(j, 0) = J0(j);
            J(j, 1) = J1(j);
            J(j, 2) = J2(j);
        }

        // Construct the inverse of the Jacobian and check for singularity. Singularities occur when the node and patch
        // are travelling at the same velocity (same direction) or if the patch is perfectly planar and the node travels
        // parallel to the plane. Both cases mean no force resolution is needed and will return a false condition. These
        // conditions can be seen in singularity_detection_check.py in the python version.
        J_det = det(J);
        if (fabs(J_det) < tol)
        {
            return false;
        }

        inv(J, J_inv, J_det);
        mat_mul(J_inv, F, grad);
        for (int j = 0; j < 3; j++)
        {
            sol[j] = sol[j] - grad(j);
        }
        // update xi, eta, and del_tc
        xi_val = sol[0];
        eta_val = sol[1];
        del_tc = sol[2];
    }  // end solver loop

    if (iters == max_iter - 1)
    {
        return false;
    } else
    {
        return true;
    }
}  // end get_contact_point

KOKKOS_FUNCTION
bool contact_patch_t::contact_check(const contact_node_t &node, const double &del_t, double &xi_val, double &eta_val,
                                    double &del_tc) const
{
    // Constructing the guess value
    // The guess is determined by projecting the node onto a plane formed by the patch at del_t/2.
    // First, compute the centroid of the patch at time del_t/2
    double A_arr[3*contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> A(&A_arr[0], 3, num_nodes_in_patch);
    construct_basis(A, del_t/2);

    double centroid[3];
    for (int i = 0; i < 3; i++)
    {
        centroid[i] = 0.0;
        for (int j = 0; j < num_nodes_in_patch; j++)
        {
            centroid[i] += A(i, j);
        }
        centroid[i] /= num_nodes_in_patch;
    }

    // Compute the position of the penetrating node at del_t/2
    double node_later[3];
    for (int i = 0; i < 3; i++)
    {
        node_later[i] = node.pos(i) + node.vel(i)*del_t/2 + 0.25*node.acc(i)*del_t*del_t;
    }

    // Construct the basis vectors. The first row of the matrix is the vector from the centroid to the reference point
    // (1, 0) on the patch. The second row of the matrix is the vector from the centroid to the reference point (0, 1).
    // The basis matrix is a 2x3 matrix always.
    double b1_arr[3];
    ViewCArrayKokkos<double> b1(&b1_arr[0], 3);
    double b2_arr[3];
    ViewCArrayKokkos<double> b2(&b2_arr[0], 3);
    double p1_arr[2];
    ViewCArrayKokkos<double> p1(&p1_arr[0], 2);
    p1(0) = 1.0;
    p1(1) = 0.0;
    double p2_arr[2];
    ViewCArrayKokkos<double> p2(&p2_arr[0], 2);
    p2(0) = 0.0;
    p2(1) = 1.0;
    ref_to_physical(p1, A, b1);
    ref_to_physical(p2, A, b2);

    // Get b1, b2, and node_later relative to centroid
    ViewCArrayKokkos<double> v(&node_later[0], 3);  // position of node relative to centroid
    for (int i = 0; i < 3; i++)
    {
        b1(i) -= centroid[i];
        b2(i) -= centroid[i];
        v(i) -= centroid[i];
    }

    // b1 and b2 need to be unit vectors to ensure that the guess values are between -1 and 1.
    double b1_norm = norm(b1);
    double b2_norm = norm(b2);
    for (int i = 0; i < 3; i++)
    {
        b1(i) /= b1_norm;
        b2(i) /= b2_norm;
    }
    // v also needs to be a normal vector, but if its norm is zero, then we leave it as is.
    double v_norm = norm(v);
    if (v_norm != 0.0)
    {
        for (int i = 0; i < 3; i++)
        {
            v(i) /= v_norm;
        }
    }

    // Get A_basis, which is the basis vectors found above.
    double A_basis_arr[2*3];
    ViewCArrayKokkos<double> A_basis(&A_basis_arr[0], 2, 3);
    for (int i = 0; i < 3; i++)
    {
        A_basis(0, i) = b1(i);
        A_basis(1, i) = b2(i);
    }

    // The matrix multiplication of A_basis*v is the projection of the node onto the plane formed by the patch.
    double guess_arr[2];
    ViewCArrayKokkos<double> guess(&guess_arr[0], 2);
    mat_mul(A_basis, v, guess);
    xi_val = guess(0);
    eta_val = guess(1);
    del_tc = del_t/2;

    // Get the solution
    bool solution_found = get_contact_point(node, xi_val, eta_val, del_tc);

    if (solution_found && fabs(xi_val) <= 1.0 + edge_tol && fabs(eta_val) <= 1.0 + edge_tol && del_tc >= 0.0 - tol &&
        del_tc <= del_t + tol)
    {
        return true;
    } else
    {
        return false;
    }
}  // end contact_check

KOKKOS_FUNCTION
void contact_patch_t::ref_to_physical(const ViewCArrayKokkos<double> &ref, const ViewCArrayKokkos<double> &A,
                                      ViewCArrayKokkos<double> &phys) const
{
    const double &xi_ = ref(0);
    const double &eta_ = ref(1);

    double phi_k_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> phi_k(&phi_k_arr[0], num_nodes_in_patch);
    phi(phi_k, xi_, eta_);

    mat_mul(A, phi_k, phys);
}

KOKKOS_FUNCTION
void contact_patch_t::phi(ViewCArrayKokkos<double> &phi_k, const double &xi_value, const double &eta_value) const
{
    if (num_nodes_in_patch == 4)
    {
        for (int i = 0; i < 4; i++)
        {
            phi_k(i) = 0.25*(1.0 + xi(i)*xi_value)*(1.0 + eta(i)*eta_value);
        }
    } else
    {
        std::cerr << "Error: higher order elements are not yet tested for contact" << std::endl;
        exit(1);
    }
}  // end phi

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
KOKKOS_FUNCTION
void contact_patch_t::d_phi_d_xi(ViewCArrayKokkos<double> &d_phi_k_d_xi, const double &xi_value,
                                 const double &eta_value) const
{
    // xi_value is used for higher order elements
    if (num_nodes_in_patch == 4)
    {
        for (int i = 0; i < 4; i++)
        {
            d_phi_k_d_xi(i) = 0.25*xi(i)*(1.0 + eta(i)*eta_value);
        }
    } else
    {
        std::cerr << "Error: higher order elements are not yet tested for contact" << std::endl;
        exit(1);
    }
}  // end d_phi_d_xi

KOKKOS_FUNCTION
void contact_patch_t::d_phi_d_eta(ViewCArrayKokkos<double> &d_phi_k_d_eta, const double &xi_value,
                                  const double &eta_value) const
{
    // eta_value is used for higher order elements
    if (num_nodes_in_patch == 4)
    {
        for (int i = 0; i < 4; i++)
        {
            d_phi_k_d_eta(i) = 0.25*(1.0 + xi(i)*xi_value)*eta(i);
        }
    } else
    {
        std::cerr << "Error: higher order elements are not yet tested for contact" << std::endl;
        exit(1);
    }
}  // end d_phi_d_eta

KOKKOS_FUNCTION
void contact_patch_t::get_normal(const double &xi_val, const double &eta_val, const double &del_t,
                                 ViewCArrayKokkos<double> &normal) const
{
    // The normal is defined as the cross product between dr/dxi and dr/deta where r is the position vector, that is
    // r = A*phi_k.

    // Get the derivative arrays
    double d_phi_d_xi_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_phi_d_xi_(&d_phi_d_xi_arr[0], num_nodes_in_patch);
    d_phi_d_xi(d_phi_d_xi_, xi_val, eta_val);

    double d_phi_d_eta_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_phi_d_eta_(&d_phi_d_eta_arr[0], num_nodes_in_patch);
    d_phi_d_eta(d_phi_d_eta_, xi_val, eta_val);

    // Construct the basis matrix A
    double A_arr[3*contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> A(&A_arr[0], 3, num_nodes_in_patch);
    construct_basis(A, del_t);

    // Get dr_dxi and dr_deta by performing the matrix multiplication A*d_phi_d_xi and A*d_phi_d_eta
    double dr_dxi_arr[3];
    ViewCArrayKokkos<double> dr_dxi(&dr_dxi_arr[0], 3);
    mat_mul(A, d_phi_d_xi_, dr_dxi);

    double dr_deta_arr[3];
    ViewCArrayKokkos<double> dr_deta(&dr_deta_arr[0], 3);
    mat_mul(A, d_phi_d_eta_, dr_deta);

    // Get the normal by performing the cross product between dr_dxi and dr_deta
    normal(0) = dr_dxi(1)*dr_deta(2) - dr_dxi(2)*dr_deta(1);
    normal(1) = dr_dxi(2)*dr_deta(0) - dr_dxi(0)*dr_deta(2);
    normal(2) = dr_dxi(0)*dr_deta(1) - dr_dxi(1)*dr_deta(0);

    // Make the normal a unit vector
    double norm_val = norm(normal);
    for (int i = 0; i < 3; i++)
    {
        normal(i) /= norm_val;
    }
}  // end get_normal

#pragma clang diagnostic pop
/// end of contact_patch_t member functions ////////////////////////////////////////////////////////////////////////////

/// beginning of contact_pair_t member functions ///////////////////////////////////////////////////////////////////////
contact_pair_t::contact_pair_t()
{
    // Default constructor
}

KOKKOS_FUNCTION
contact_pair_t::contact_pair_t(contact_patches_t &contact_patches_obj, const contact_patch_t &patch_obj,
                               const contact_node_t &node_obj, const double &xi_val, const double &eta_val,
                               const double &del_tc_val, const ViewCArrayKokkos<double> &normal_view)
{
    // Set the contact_pair_t members
    // todo: patch and node might need to be pointers instead so that changes to the patch and node objects are
    //       reflected in contact_patches_t::contact_nodes and contact_patches_t::contact_patches; however, I think that
    //       the patch and node objects references in the call mean that this is already happening.
    patch = patch_obj;
    node = node_obj;
    xi = xi_val;
    eta = eta_val;
    del_tc = del_tc_val;
    normal(0) = normal_view(0);
    normal(1) = normal_view(1);
    normal(2) = normal_view(2);

    contact_patches_obj.is_pen_node(node.gid) = true;
    for (int i = 0; i < contact_patch_t::num_nodes_in_patch; i++)
    {
        contact_patches_obj.is_patch_node(patch.nodes_gid(i)) = true;
    }

    // Add the pair to the contact_pairs_access
    size_t &patch_stride = contact_patches_obj.contact_pairs_access.stride(patch.lid);
    patch_stride++;
    contact_patches_obj.contact_pairs_access(patch.lid, patch_stride - 1) = node.gid;
}

KOKKOS_FUNCTION
void contact_pair_t::frictionless_increment(const contact_patches_t &contact_patches, const double &del_t)
{
    // In order to understand this, just see this PDF:
    // https://github.com/gabemorris12/contact_surfaces/blob/master/Finding%20the%20Contact%20Force.pdf

    double A_arr[3*contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> A(&A_arr[0], 3, contact_patch_t::num_nodes_in_patch);

    double phi_k_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> phi_k(&phi_k_arr[0], contact_patch_t::num_nodes_in_patch);

    double d_phi_d_xi_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_phi_d_xi_(&d_phi_d_xi_arr[0], contact_patch_t::num_nodes_in_patch);

    double d_phi_d_eta_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> d_phi_d_eta_(&d_phi_d_eta_arr[0], contact_patch_t::num_nodes_in_patch);

    double ak;  // place to store acceleration of a patch node
    double as;  // place to store acceleration of the penetrating node

    double rhs_arr[3];  // right hand side (A_arr*phi_k)
    ViewCArrayKokkos<double> rhs(&rhs_arr[0], 3);

    double lhs;  // left hand side (node.pos + node.vel*del_t + 0.5*node.acc*del_t*del_t)

    double F_arr[3];  // defined as lhs - rhs
    ViewCArrayKokkos<double> F(&F_arr[0], 3);

    double d_A_d_xi_arr[3*contact_patch_t::max_nodes];  // derivative of A wrt xi
    ViewCArrayKokkos<double> d_A_d_xi(&d_A_d_xi_arr[0], 3, contact_patch_t::num_nodes_in_patch);

    double d_A_d_eta_arr[3*contact_patch_t::max_nodes];  // derivative of A wrt eta
    ViewCArrayKokkos<double> d_A_d_eta(&d_A_d_eta_arr[0], 3, contact_patch_t::num_nodes_in_patch);

    double d_A_d_fc_arr[3*contact_patch_t::max_nodes];  // derivative of A wrt fc_inc
    ViewCArrayKokkos<double> d_A_d_fc(&d_A_d_fc_arr[0], 3, contact_patch_t::num_nodes_in_patch);

    double neg_normal_arr[3];
    ViewCArrayKokkos<double> neg_normal(&neg_normal_arr[0], 3);
    for (int i = 0; i < 3; i++)
    {
        neg_normal(i) = -normal(i);
    }

    double outer1_arr[contact_patch_t::max_nodes];  // right segment in first outer product
    ViewCArrayKokkos<double> outer1(&outer1_arr[0], contact_patch_t::num_nodes_in_patch);

    double outer2_arr[contact_patch_t::max_nodes];  // right segment in second outer product
    ViewCArrayKokkos<double> outer2(&outer2_arr[0], contact_patch_t::num_nodes_in_patch);

    double outer3_arr[contact_patch_t::max_nodes];  // right segment in third outer product
    ViewCArrayKokkos<double> outer3(&outer3_arr[0], contact_patch_t::num_nodes_in_patch);

    double J0_first_arr[3];  // first term in the J0 column calculation
    ViewCArrayKokkos<double> J0_first(&J0_first_arr[0], 3);

    double J0_second_arr[3];  // second term in the J0 column calculation
    ViewCArrayKokkos<double> J0_second(&J0_second_arr[0], 3);

    double J1_first_arr[3];  // first term in the J1 column calculation
    ViewCArrayKokkos<double> J1_first(&J1_first_arr[0], 3);

    double J1_second_arr[3];  // second term in the J1 column calculation
    ViewCArrayKokkos<double> J1_second(&J1_second_arr[0], 3);

    double J2_second_arr[3];  // second term in the J2 column calculation
    ViewCArrayKokkos<double> J2_second(&J2_second_arr[0], 3);

    double J0_arr[3];  // column 1 of jacobian
    ViewCArrayKokkos<double> J0(&J0_arr[0], 3);

    double J1_arr[3];  // column 2 of jacobian
    ViewCArrayKokkos<double> J1(&J1_arr[0], 3);

    double J2_arr[3];  // column 3 of jacobian
    ViewCArrayKokkos<double> J2(&J2_arr[0], 3);

    double J_arr[9];  // jacobian
    ViewCArrayKokkos<double> J(&J_arr[0], 3, 3);

    double J_inv_arr[9];  // inverse of jacobian
    ViewCArrayKokkos<double> J_inv(&J_inv_arr[0], 3, 3);

    double J_det;  // determinant of jacobian
    double sol[3];
    sol[0] = xi;
    sol[1] = eta;
    sol[2] = fc_inc;

    double grad_arr[3];  // J_inv*F term
    ViewCArrayKokkos<double> grad(&grad_arr[0], 3);

    for (int i = 0; i < max_iter; i++)
    {
        patch.phi(phi_k, xi, eta);
        // construct A
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
            {
                const contact_node_t &patch_node = contact_patches.contact_nodes(patch.nodes_gid(k));
                ak = (patch.internal_force(j, k) - fc_inc*normal(j)*phi_k(k) +
                      patch_node.contact_force(j))/patch.mass_points(j, k);
                A(j, k) = patch.points(j, k) + patch.vel_points(j, k)*del_t + 0.5*ak*del_t*del_t;
            }
        }

        // construct F
        mat_mul(A, phi_k, rhs);
        for (int j = 0; j < 3; j++)
        {
            as = (node.internal_force(j) + fc_inc*normal(j) + node.contact_force(j))/node.mass;
            lhs = node.pos(j) + node.vel(j)*del_t + 0.5*as*del_t*del_t;
            F(j) = lhs - rhs(j);
        }

        if (norm(F) <= tol)
        {
            break;
        }

        // construct J
        patch.d_phi_d_xi(d_phi_d_xi_, xi, eta);
        patch.d_phi_d_eta(d_phi_d_eta_, xi, eta);

        for (int j = 0; j < contact_patch_t::num_nodes_in_patch; j++)
        {
            outer1(j) = (0.5*d_phi_d_xi_(j)*fc_inc*del_t*del_t)/patch.mass_points(0, j);
            outer2(j) = (0.5*d_phi_d_eta_(j)*fc_inc*del_t*del_t)/patch.mass_points(0, j);
            outer3(j) = (0.5*phi_k(j)*del_t*del_t)/patch.mass_points(0, j);
        }

        outer(neg_normal, outer1, d_A_d_xi);
        outer(neg_normal, outer2, d_A_d_eta);
        outer(neg_normal, outer3, d_A_d_fc);

        mat_mul(A, d_phi_d_xi_, J0_first);
        mat_mul(d_A_d_xi, phi_k, J0_second);
        for (int j = 0; j < 3; j++)
        {
            J0(j) = -J0_first(j) - J0_second(j);
        }

        mat_mul(A, d_phi_d_eta_, J1_first);
        mat_mul(d_A_d_eta, phi_k, J1_second);
        for (int j = 0; j < 3; j++)
        {
            J1(j) = -J1_first(j) - J1_second(j);
        }

        mat_mul(d_A_d_fc, phi_k, J2_second);
        for (int j = 0; j < 3; j++)
        {
            J2(j) = (0.5*del_t*del_t*normal(j))/node.mass - J2_second(j);
        }

        for (int j = 0; j < 3; j++)
        {
            J(j, 0) = J0(j);
            J(j, 1) = J1(j);
            J(j, 2) = J2(j);
        }

        J_det = det(J);  // there should be no singularities in this calculation
        if (fabs(J_det) < tol)
        {
            fc_inc = 0.0;
            printf("Error: Singularity detected in frictionless_increment\n");
            break;
        }

        inv(J, J_inv, J_det);
        mat_mul(J_inv, F, grad);
        for (int j = 0; j < 3; j++)
        {
            sol[j] = sol[j] - grad(j);
        }
        xi = sol[0];
        eta = sol[1];
        fc_inc = sol[2];
    }
}

KOKKOS_FUNCTION
void contact_pair_t::distribute_frictionless_force(contact_patches_t &contact_patches, const double &force_scale)
{
    double force_val = force_scale*fc_inc;

    // get phi_k
    double phi_k_arr[contact_patch_t::max_nodes];
    ViewCArrayKokkos<double> phi_k(&phi_k_arr[0], contact_patch_t::num_nodes_in_patch);
    patch.phi(phi_k, xi, eta);

    // if tensile, then subtract left over fc_inc_total; if not, then distribute to nodes
    if (force_val + fc_inc_total < 0.0)
    {
        // update penetrating node
        for (int i = 0; i < 3; i++)
        {
            node.contact_force(i) -= fc_inc_total*normal(i);
        }

        // update patch nodes
        for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
        {
            contact_node_t &patch_node = contact_patches.contact_nodes(patch.nodes_gid(k));
            for (int i = 0; i < 3; i++)
            {
                patch_node.contact_force(i) += fc_inc_total*normal(i)*phi_k(k);
            }
        }

        fc_inc_total = 0.0;
        fc_inc = 0.0;
    } else
    {
        fc_inc_total += force_val;

        // update penetrating node
        for (int i = 0; i < 3; i++)
        {
            node.contact_force(i) += force_val*normal(i);
        }

        // update patch nodes
        for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
        {
            contact_node_t &patch_node = contact_patches.contact_nodes(patch.nodes_gid(k));
            for (int i = 0; i < 3; i++)
            {
                patch_node.contact_force(i) -= force_val*normal(i)*phi_k(k);
            }
        }
    }
}  // end distribute_frictionless_force

bool contact_pair_t::should_remove(const double &del_t)
{
    if (fc_inc_total == 0.0 || fabs(xi) > 1.0 + edge_tol || fabs(eta) > 1.0 + edge_tol)
    {
        return true;
    } else
    {
        // update the normal
        double new_normal_arr[3];
        ViewCArrayKokkos<double> new_normal(&new_normal_arr[0], 3);
        patch.get_normal(xi, eta, del_t, new_normal);
        for (int i = 0; i < 3; i++)
        {
            normal(i) = new_normal(i);
        }

        return false;
    }
}  // end should_remove
/// end of contact_pair_t member functions /////////////////////////////////////////////////////////////////////////////

/// beginning of contact_patches_t member functions ////////////////////////////////////////////////////////////////////
void contact_patches_t::initialize(const mesh_t &mesh, const CArrayKokkos<size_t> &bdy_contact_patches,
                                   const node_t &nodes)
{
    // Contact is only supported in 3D
    if (mesh.num_dims != 3)
    {
        std::cerr << "Error: contact is only supported in 3D" << std::endl;
        exit(1);
    }

    // Set up the patches that will be checked for contact
    patches_gid = bdy_contact_patches;
    num_contact_patches = patches_gid.size();

    // Set up array of contact_patch_t
    contact_patches = CArrayKokkos<contact_patch_t>(num_contact_patches);

    CArrayKokkos<size_t> nodes_in_patch(num_contact_patches, mesh.num_nodes_in_patch);
    FOR_ALL_CLASS(i, 0, num_contact_patches,
                  j, 0, mesh.num_nodes_in_patch, {
                      nodes_in_patch(i, j) = mesh.nodes_in_patch(patches_gid(i), j);
                  });  // end parallel for
    Kokkos::fence();

    // todo: Kokkos arrays are being accessed and modified but this isn't inside a macro. Should this be inside Run()?
    for (int i = 0; i < num_contact_patches; i++)
    {
        contact_patch_t &contact_patch = contact_patches(i);
        contact_patch.gid = patches_gid(i);
        contact_patch.lid = i;

        // Make contact_patch.nodes_gid equal to the row of nodes_in_patch(i)
        // This line is what is limiting the parallelism
        contact_patch.nodes_gid = CArrayKokkos<size_t>(mesh.num_nodes_in_patch);
        contact_patch.nodes_obj = CArrayKokkos<contact_node_t>(mesh.num_nodes_in_patch);
        for (size_t j = 0; j < mesh.num_nodes_in_patch; j++)
        {
            contact_patch.nodes_gid(j) = nodes_in_patch(i, j);
        }
    }  // end for

    // todo: This if statement might need a closer look
    // Setting up the iso-parametric coordinates for all patch objects
    if (mesh.num_nodes_in_patch == 4)
    {
        contact_patch_t::num_nodes_in_patch = 4;
        double xi_temp[4] = {-1.0, 1.0, 1.0, -1.0};
        double eta_temp[4] = {-1.0, -1.0, 1.0, 1.0};

        // Allocating memory for the points and vel_points arrays
        for (int i = 0; i < num_contact_patches; i++)
        {
            contact_patch_t &contact_patch = contact_patches(i);
            contact_patch.points = CArrayKokkos<double>(3, 4);
            contact_patch.vel_points = CArrayKokkos<double>(3, 4);
            contact_patch.internal_force = CArrayKokkos<double>(3, 4);
            contact_patch.acc_points = CArrayKokkos<double>(3, 4);
            contact_patch.mass_points = CArrayKokkos<double>(3, 4);

            contact_patch.xi = CArrayKokkos<double>(4);
            contact_patch.eta = CArrayKokkos<double>(4);

            // todo: We have the Kokkos xi and eta members being modified on host. I think it may be that the xi_temp
            //       and eta_temp arrays need to be ridden and do a serial Run() for the construction of xi and eta.
            for (int j = 0; j < 4; j++)
            {
                contact_patch.xi(j) = xi_temp[j];
                contact_patch.eta(j) = eta_temp[j];
            }
        }

    } else
    {
        std::cerr << "Error: higher order elements are not yet tested for contact" << std::endl;
        exit(1);
    }  // end if

    // Determine the bucket size. This is defined as 1.001*min_node_distance
    CArrayKokkos<double> node_distances(num_contact_patches, contact_patch_t::num_nodes_in_patch);
    FOR_ALL_CLASS(i, 0, num_contact_patches,
                  j, 0, contact_patch_t::num_nodes_in_patch, {
                      const contact_patch_t &contact_patch = contact_patches(i);
                      if (j < contact_patch_t::num_nodes_in_patch - 1)
                      {
                          const size_t n1 = contact_patch.nodes_gid(j); // current node
                          const size_t n2 = contact_patch.nodes_gid(j + 1); // next node

                          double sum_sq = 0.0;

                          for (int k = 0; k < 3; k++)
                          {
                              sum_sq += pow(nodes.coords(0, n1, k) - nodes.coords(0, n2, k), 2);
                          }

                          node_distances(i, j) = sqrt(sum_sq);
                      } else
                      {
                          const size_t n1 = contact_patch.nodes_gid(0); // current node
                          const size_t n2 = contact_patch.nodes_gid(
                              contact_patch_t::num_nodes_in_patch - 1); // next node

                          double sum_sq = 0.0;

                          for (int k = 0; k < 3; k++)
                          {
                              sum_sq += pow(nodes.coords(0, n1, k) - nodes.coords(0, n2, k), 2);
                          }

                          node_distances(i, j) = sqrt(sum_sq);
                      }
                  });
    Kokkos::fence();

    double result = 0.0;
    double local_min = 1.0e10;
    REDUCE_MIN(i, 0, num_contact_patches,
               j, 0, contact_patch_t::num_nodes_in_patch, local_min, {
                   if (local_min > node_distances(i, j))
                   {
                       local_min = node_distances(i, j);
                   }
               }, result);

    contact_patches_t::bucket_size = 0.999*result;

    // Find the total number of nodes (this is should always be less than or equal to mesh.num_bdy_nodes)
    size_t local_max_index = 0;
    size_t max_index = 0;
    REDUCE_MAX(i, 0, num_contact_patches,
               j, 0, contact_patch_t::num_nodes_in_patch, local_max_index, {
                   if (local_max_index < contact_patches(i).nodes_gid(j))
                   {
                       local_max_index = contact_patches(i).nodes_gid(j);
                   }
               }, max_index);
    Kokkos::fence();

    CArrayKokkos<size_t> node_count(max_index + 1);
    for (int i = 0; i < num_contact_patches; i++)
    {
        contact_patch_t &contact_patch = contact_patches(i);
        FOR_ALL(j, 0, contact_patch_t::num_nodes_in_patch, {
            size_t node_gid = contact_patch.nodes_gid(j);
            if (node_count(node_gid) == 0)
            {
                node_count(node_gid) = 1;
                contact_patches_t::num_contact_nodes += 1;
            }
        });
    }

    // Initialize the contact_nodes and contact_pairs arrays
    contact_nodes = CArrayKokkos<contact_node_t>(max_index + 1);
    contact_pairs = CArrayKokkos<contact_pair_t>(max_index + 1);
    contact_pairs_access = DynamicRaggedRightArrayKokkos<size_t>(num_contact_patches,
                                                                 contact_patch_t::max_contacting_nodes_in_patch);
    is_patch_node = CArrayKokkos<bool>(max_index + 1);
    is_pen_node = CArrayKokkos<bool>(max_index + 1);
    active_pairs = CArrayKokkos<size_t>(contact_patches_t::num_contact_nodes);
    forces = CArrayKokkos<double>(contact_patches_t::num_contact_nodes);

    // Construct nodes_gid and nodes_obj
    nodes_gid = CArrayKokkos<size_t>(contact_patches_t::num_contact_nodes);
    size_t node_lid = 0;
    for (int i = 0; i < num_contact_patches; i++)
    {
        contact_patch_t &contact_patch = contact_patches(i);
        for (int j = 0; j < contact_patch_t::num_nodes_in_patch; j++)
        {
            size_t node_gid = contact_patch.nodes_gid(j);
            const contact_node_t &node_obj = contact_nodes(node_gid);
            contact_patch.nodes_obj(j) = node_obj;
            if (node_count(node_gid) == 1)
            {
                node_count(node_gid) = 2;
                nodes_gid(node_lid) = node_gid;
                node_lid += 1;
            }
        }
    }

    // Construct patches_in_node and num_patches_in_node
    num_patches_in_node = CArrayKokkos<size_t>(max_index + 1);
    for (int patch_lid = 0; patch_lid < num_contact_patches; patch_lid++)
    {
        const contact_patch_t &contact_patch = contact_patches(patch_lid);
        // for each node in the patch, increment the counter in num_patches_in_node
        FOR_ALL_CLASS(i, 0, contact_patch_t::num_nodes_in_patch, {
            const size_t &node_gid = contact_patch.nodes_gid(i);
            num_patches_in_node(node_gid) += 1;
        });
        Kokkos::fence();
    }

    CArrayKokkos<size_t> stride_index(max_index + 1);
    patches_in_node = RaggedRightArrayKokkos<size_t>(num_patches_in_node);
    // Walk through the patches, and for each node in the patch, add the patch to the patches_in_node array
    for (int patch_lid = 0; patch_lid < num_contact_patches; patch_lid++)
    {
        // todo: will the device have access to patch_lid as I have it here?
        const contact_patch_t &contact_patch = contact_patches(patch_lid);
        FOR_ALL_CLASS(i, 0, contact_patch_t::num_nodes_in_patch, {
            const size_t &node_gid = contact_patch.nodes_gid(i);
            const size_t &stride = stride_index(node_gid);
            patches_in_node(node_gid, stride) = patch_lid;
            stride_index(node_gid) += 1;
        });
        Kokkos::fence();
    }

}  // end initialize


void contact_patches_t::sort(const mesh_t &mesh, const node_t &nodes, const corner_t &corner)
{
    // Update the points and vel_points arrays for each contact patch
    FOR_ALL_CLASS(i, 0, num_contact_patches, {
        contact_patches(i).update_nodes(mesh, nodes, corner);
    });  // end parallel for
    Kokkos::fence();

    // Update node objects
    FOR_ALL_CLASS(i, 0, contact_patches_t::num_contact_nodes, {
        const size_t &node_gid = nodes_gid(i);
        contact_node_t &contact_node = contact_nodes(node_gid);
        contact_node.gid = node_gid;
        contact_node.mass = nodes.mass(node_gid);

        // Update pos, vel, acc, and internal force
        for (int j = 0; j < 3; j++)
        {
            contact_node.pos(j) = nodes.coords(0, node_gid, j);
            contact_node.vel(j) = nodes.vel(0, node_gid, j);

            // zero forces
            contact_node.contact_force(j) = 0.0;
            contact_node.internal_force(j) = 0.0;

            // Loop over the corners
            for (size_t corner_lid = 0; corner_lid < mesh.num_corners_in_node(node_gid); corner_lid++)
            {
                size_t corner_gid = mesh.corners_in_node(node_gid, corner_lid);
                contact_node.internal_force(j) += corner.force(corner_gid, j);
            }

            contact_node.acc(j) = contact_node.internal_force(j)/contact_node.mass;
        }
    });

    // todo: I don't think it's a good idea to have these allocated here. These should become member variables, and
    //       its allocation should be moved to initialize() because sort() is being called every step.
    // Grouping all the coordinates, velocities, and accelerations
    CArrayKokkos<double> points(3, contact_patch_t::num_nodes_in_patch*num_contact_patches);
    CArrayKokkos<double> velocities(3, contact_patch_t::num_nodes_in_patch*num_contact_patches);
    CArrayKokkos<double> accelerations(3, contact_patch_t::num_nodes_in_patch*num_contact_patches);

    FOR_ALL(i, 0, num_contact_patches, {
        const contact_patch_t &contact_patch = contact_patches(i);
        for (int j = 0; j < contact_patch_t::num_nodes_in_patch; j++)
        {
            const size_t node_gid = contact_patch.nodes_gid(j);
            const double &mass = nodes.mass(node_gid);
            for (int k = 0; k < 3; k++)
            {
                points(k, i*contact_patch_t::num_nodes_in_patch + j) = contact_patch.points(k, j);
                velocities(k, i*contact_patch_t::num_nodes_in_patch + j) = fabs(contact_patch.vel_points(k, j));
                accelerations(k, i*contact_patch_t::num_nodes_in_patch + j) = fabs(
                    contact_patch.internal_force(k, j)/mass);
            }
        }
    });  // end parallel for
    Kokkos::fence();

    // Find the max and min for each dimension
    double local_x_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_x_max, {
        if (local_x_max < points(0, i))
        {
            local_x_max = points(0, i);
        }
    }, x_max);
    double local_y_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_y_max, {
        if (local_y_max < points(1, i))
        {
            local_y_max = points(1, i);
        }
    }, y_max);
    double local_z_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_z_max, {
        if (local_z_max < points(2, i))
        {
            local_z_max = points(2, i);
        }
    }, z_max);
    double local_x_min = 0.0;
    REDUCE_MIN_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_x_min, {
        if (local_x_min > points(0, i))
        {
            local_x_min = points(0, i);
        }
    }, x_min);
    double local_y_min = 0.0;
    REDUCE_MIN_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_y_min, {
        if (local_y_min > points(1, i))
        {
            local_y_min = points(1, i);
        }
    }, y_min);
    double local_z_min = 0.0;
    REDUCE_MIN_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_z_min, {
        if (local_z_min > points(2, i))
        {
            local_z_min = points(2, i);
        }
    }, z_min);
    double local_vx_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_vx_max, {
        if (local_vx_max < velocities(0, i))
        {
            local_vx_max = velocities(0, i);
        }
    }, vx_max);
    double local_vy_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_vy_max, {
        if (local_vy_max < velocities(1, i))
        {
            local_vy_max = velocities(1, i);
        }
    }, vy_max);
    double local_vz_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_vz_max, {
        if (local_vz_max < velocities(2, i))
        {
            local_vz_max = velocities(2, i);
        }
    }, vz_max);
    double local_ax_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_ax_max, {
        if (local_ax_max < accelerations(0, i))
        {
            local_ax_max = accelerations(0, i);
        }
    }, ax_max);
    double local_ay_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_ay_max, {
        if (local_ay_max < accelerations(1, i))
        {
            local_ay_max = accelerations(1, i);
        }
    }, ay_max);
    double local_az_max = 0.0;
    REDUCE_MAX_CLASS(i, 0, contact_patch_t::num_nodes_in_patch*num_contact_patches, local_az_max, {
        if (local_az_max < accelerations(2, i))
        {
            local_az_max = accelerations(2, i);
        }
    }, az_max);
    Kokkos::fence();

    // If the max velocity is zero, then we want to set it to a small value. The max velocity and acceleration are used
    // for creating a capture box around the contact patch. We want there to be at least some thickness to the box.
    double* vel_max[3] = {&vx_max, &vy_max, &vz_max};
    for (auto &i: vel_max)
    {
        if (*i == 0.0)
        {
            *i = 1.0e-3; // Set to a small value
        }
    }

    // Define Sx, Sy, and Sz
    Sx = floor((x_max - x_min)/bucket_size) + 1; // NOLINT(*-narrowing-conversions)
    Sy = floor((y_max - y_min)/bucket_size) + 1; // NOLINT(*-narrowing-conversions)
    Sz = floor((z_max - z_min)/bucket_size) + 1; // NOLINT(*-narrowing-conversions)

    // todo: Something similar to the points, velocities, and accelerations arrays above should be done here. The issue
    //       with this one is that nb changes through the iterations. lbox and npoint might need to change to Views.
    // Initializing the nbox, lbox, nsort, and npoint arrays
    size_t nb = Sx*Sy*Sz;  // total number of buckets
    nbox = CArrayKokkos<size_t>(nb);
    lbox = CArrayKokkos<size_t>(contact_patches_t::num_contact_nodes);
    nsort = CArrayKokkos<size_t>(contact_patches_t::num_contact_nodes);
    npoint = CArrayKokkos<size_t>(nb);
    CArrayKokkos<size_t> nsort_lid(contact_patches_t::num_contact_nodes);

    // Find the bucket id for each node by constructing lbox
    FOR_ALL_CLASS(i, 0, contact_patches_t::num_contact_nodes, {
        size_t node_gid = nodes_gid(i);
        double x = nodes.coords(0, node_gid, 0);
        double y = nodes.coords(0, node_gid, 1);
        double z = nodes.coords(0, node_gid, 2);

        size_t Si_x = floor((x - x_min)/bucket_size);
        size_t Si_y = floor((y - y_min)/bucket_size);
        size_t Si_z = floor((z - z_min)/bucket_size);

        lbox(i) = Si_z*Sx*Sy + Si_y*Sx + Si_x;
        nbox(lbox(i)) += 1;  // increment nbox
    });
    Kokkos::fence();

    // Calculate the pointer for each bucket into a sorted list of nodes
    for (size_t i = 1; i < nb; i++)
    {
        npoint(i) = npoint(i - 1) + nbox(i - 1);
    }

    // Zero nbox
    FOR_ALL_CLASS(i, 0, nb, {
        nbox(i) = 0;
    });
    Kokkos::fence();

    // Sort the slave nodes according to their bucket id into nsort
    for (int i = 0; i < contact_patches_t::num_contact_nodes; i++)
    {
        nsort_lid(nbox(lbox(i)) + npoint(lbox(i))) = i;
        nbox(lbox(i)) += 1;
    }

    // Change nsort to reflect the global node id's
    FOR_ALL_CLASS(i, 0, contact_patches_t::num_contact_nodes, {
        nsort(i) = nodes_gid(nsort_lid(i));
    });
    Kokkos::fence();
}  // end sort

void contact_patches_t::find_nodes(contact_patch_t &contact_patch, const double &del_t,
                                   size_t &num_nodes_found)
{
    // Get capture box
    contact_patch.capture_box(vx_max, vy_max, vz_max, ax_max, ay_max, az_max, del_t);
    CArrayKokkos<double> &bounds = contact_patch.bounds;

    // Determine the buckets that intersect with the capture box
    size_t ibox_max = fmax(0, fmin(Sx - 1, floor((bounds(0) - x_min)/bucket_size))); // NOLINT(*-narrowing-conversions)
    size_t jbox_max = fmax(0, fmin(Sy - 1, floor((bounds(1) - y_min)/bucket_size))); // NOLINT(*-narrowing-conversions)
    size_t kbox_max = fmax(0, fmin(Sz - 1, floor((bounds(2) - z_min)/bucket_size))); // NOLINT(*-narrowing-conversions)
    size_t ibox_min = fmax(0, fmin(Sx - 1, floor((bounds(3) - x_min)/bucket_size))); // NOLINT(*-narrowing-conversions)
    size_t jbox_min = fmax(0, fmin(Sy - 1, floor((bounds(4) - y_min)/bucket_size))); // NOLINT(*-narrowing-conversions)
    size_t kbox_min = fmax(0, fmin(Sz - 1, floor((bounds(5) - z_min)/bucket_size))); // NOLINT(*-narrowing-conversions)

    size_t bucket_index = 0;
    for (size_t i = ibox_min; i < ibox_max + 1; i++)
    {
        for (size_t j = jbox_min; j < jbox_max + 1; j++)
        {
            for (size_t k = kbox_min; k < kbox_max + 1; k++)
            {
                contact_patch.buckets(bucket_index) = k*Sx*Sy + j*Sx + i;
                bucket_index += 1;
            }
        }
    }

    // Get all nodes in each bucket
    num_nodes_found = 0;  // local node index for possible_nodes member
    for (int bucket_lid = 0; bucket_lid < bucket_index; bucket_lid++)
    {
        size_t b = contact_patch.buckets(bucket_lid);
        for (size_t i = 0; i < nbox(b); i++)
        {
            size_t node_gid = nsort(npoint(b) + i);
            bool add_node = true;
            // If the node is in the current contact_patch, then continue; else, add it to possible_nodes
            for (int j = 0; j < contact_patch_t::num_nodes_in_patch; j++)
            {
                if (node_gid == contact_patch.nodes_gid(j))
                {
                    add_node = false;
                    break;
                }
            }

            if (add_node)
            {
                contact_patch.possible_nodes(num_nodes_found) = node_gid;
                num_nodes_found += 1;
            }
        }
    }
}  // end find_nodes

void contact_patches_t::get_contact_pairs(const double &del_t)
{
    // clear the is_patch_node and is_pen_node arrays to be false
    FOR_ALL_CLASS(i, 0, is_patch_node.size(), {
        is_patch_node(i) = false;
        is_pen_node(i) = false;
    });
    Kokkos::fence();

    for (int patch_lid = 0; patch_lid < num_contact_patches; patch_lid++)
    {
        contact_patch_t &contact_patch = contact_patches(patch_lid);
        size_t num_nodes_found;
        find_nodes(contact_patch, del_t, num_nodes_found);
        // todo: original plan was for this to be a parallel loop, but there are collisions in the contact_pairs_access
        for (int node_lid = 0; node_lid < num_nodes_found; node_lid++)
        {
            const size_t &node_gid = contact_patch.possible_nodes(node_lid);
            const contact_node_t &node = contact_nodes(node_gid);
            contact_pair_t &current_pair = contact_pairs(node_gid);
            // If this is already an active pair, then back out of the thread/continue
            if (current_pair.active)
            {
                is_pen_node(node_gid) = true;
                for (int i = 0; i < contact_patch_t::num_nodes_in_patch; i++)
                {
                    is_patch_node(contact_patch.nodes_gid(i)) = true;
                }
                // todo: ending the thread here has performance consequences for cuda; though it may be fine to do this
                //       because the number of possible nodes is small
                // return;
                continue;
            }

            double xi_val, eta_val, del_tc;
            bool is_hitting = contact_patch.contact_check(node, del_t, xi_val, eta_val, del_tc);

            if (is_hitting && !is_pen_node(node_gid) && !is_patch_node(node_gid))
            {
                double normal_arr[3];
                ViewCArrayKokkos<double> normal(&normal_arr[0], 3);
                contact_patch.get_normal(xi_val, eta_val, del_t, normal);
                current_pair = contact_pair_t(*this, contact_patch, node, xi_val, eta_val, del_tc, normal);
            } else if (is_hitting && is_pen_node(node_gid))
            {
                // This means that the current_pair has already been initialized. We need to compare the pair stored in
                // `current_pair` to the parameters in this iteration. If `current_pair.del_tc` is larger than `del_tc`
                // from this iteration, then that means that this iteration will intersect the patch first. For this,
                // we need to update the parameters in `current_pair` to reflect the ones here. The only thing that
                // stays constant is the node, while the patch, xi, eta, del_tc, etc. are updated.
                if (del_tc + tol < current_pair.del_tc)
                {
                    // see edge_cases.py for testing this branch (first test in file)
                    // remove all the patch nodes from is_patch_node
                    const contact_patch_t &original_patch = current_pair.patch;
                    for (int i = 0; i < contact_patch_t::num_nodes_in_patch; i++)
                    {
                        is_patch_node(original_patch.nodes_gid(i)) = false;
                    }

                    // todo: remove_pair might be a reason why this whole loop should be serial
                    remove_pair(current_pair);
                    double normal_arr[3];
                    ViewCArrayKokkos<double> normal(&normal_arr[0], 3);
                    contact_patch.get_normal(xi_val, eta_val, del_t, normal);
                    current_pair = contact_pair_t(*this, contact_patch, node, xi_val, eta_val, del_tc, normal);
                } else if (current_pair.del_tc - tol <= del_tc && current_pair.del_tc + tol >= del_tc)
                {
                    // see edge_cases.py second and fourth test in file

                    // This means that the node is hitting an edge. The dominant pair to be selected is based off the
                    // normal at the penetrating node's surface and the normal at the contact point.
                    double normal1_arr[3];
                    ViewCArrayKokkos<double> normal1(&normal1_arr[0], 3);
                    // current pair stores normal1
                    for (int i = 0; i < 3; i++)
                    {
                        normal1(i) = current_pair.normal(i);
                    }

                    double normal2_arr[3];
                    ViewCArrayKokkos<double> normal2(&normal2_arr[0], 3);
                    contact_patch.get_normal(xi_val, eta_val, del_t, normal2);
                    double new_normal_arr[3];
                    ViewCArrayKokkos<double> new_normal(&new_normal_arr[0], 3);
                    bool add_new_pair = get_edge_pair(normal1, normal2, node_gid, del_t, new_normal);

                    if (add_new_pair)
                    {
                        // remove all the patch nodes from is_patch_node
                        const contact_patch_t &original_patch = current_pair.patch;
                        for (int i = 0; i < contact_patch_t::num_nodes_in_patch; i++)
                        {
                            is_patch_node(original_patch.nodes_gid(i)) = false;
                        }

                        remove_pair(current_pair);
                        current_pair = contact_pair_t(*this, contact_patch, node, xi_val, eta_val, del_tc, new_normal);
                    }
                }
            } else if (is_hitting && is_patch_node(node_gid))
            {
                // This means that the current node is a patch node in a previous contact pair but is also being
                // considered as a penetrating node in this iteration. The logic with this case is loop through the
                // nodes in the patch of this iteration and see if the node_gid exists as a penetrating node in a
                // contact pair. If it does, then a comparison must be made with all the found pairs.
                bool add_current_pair = false;
                bool hitting_before_arr[contact_patch_t::max_nodes];  // stores true or false if the node of this iteration is hitting before its adjacent pairs
                ViewCArrayKokkos<bool> hitting_before(&hitting_before_arr[0], contact_patch_t::num_nodes_in_patch);
                size_t hitting_index = 0;

                for (int i = 0; i < contact_patch_t::num_nodes_in_patch; i++)
                {
                    const size_t &patch_node_gid = contact_patch.nodes_gid(i);
                    if (is_pen_node(patch_node_gid))
                    {
                        const contact_pair_t &pair = contact_pairs(patch_node_gid);  // adjacent pair
                        if (del_tc + tol < pair.del_tc)
                        {
                            // this means that the current node is hitting the patch object before the patch object
                            // node is hitting its patch
                            hitting_before(hitting_index) = true;
                            // todo: If the contact pair is a glue condition, then we need to remove the adjacent pair
                            //       here. It might be best to remove it for any case.
                        } else if (pair.del_tc - tol <= del_tc && pair.del_tc + tol >= del_tc)
                        {
                            // this means we are hitting at the same time
                            // if the contact point is not on the edge, then we hitting before is true
                            if (fabs(xi_val) < 1.0 - tol && fabs(eta_val) < 1.0 - tol)
                            {
                                hitting_before(hitting_index) = true;
                            } else
                            {
                                hitting_before(hitting_index) = false;
                            }
                        } else
                        {
                            // this means that the adjacent pair is hitting before the current node
                            hitting_before(hitting_index) = false;
                        }
                        hitting_index += 1;
                    }
                }

                if (hitting_index == 0)
                {
                    add_current_pair = true;
                } else if (all(hitting_before, hitting_index))
                {
                    add_current_pair = true;
                } else if (any(hitting_before, hitting_index) && fabs(xi_val) < 1.0 - tol &&
                           fabs(eta_val) < 1.0 - tol)
                {
                    add_current_pair = true;
                }

                if (add_current_pair)
                {
                    double normal_arr[3];
                    ViewCArrayKokkos<double> normal(&normal_arr[0], 3);
                    contact_patch.get_normal(xi_val, eta_val, del_t, normal);
                    current_pair = contact_pair_t(*this, contact_patch, node, xi_val, eta_val, del_tc, normal);
                }
            }
        }
    }

    // set the active pairs
    num_active_pairs = 0;
    for (int patch_lid = 0; patch_lid < num_contact_patches; patch_lid++)
    {
        for (int patch_stride = 0; patch_stride < contact_pairs_access.stride(patch_lid); patch_stride++)
        {
            const size_t &node_gid = contact_pairs_access(patch_lid, patch_stride);
            contact_pair_t &pair = contact_pairs(node_gid);
            pair.active = true;
            active_pairs(num_active_pairs) = node_gid;
            num_active_pairs += 1;
        }
    }
}  // end get_contact_pairs

KOKKOS_FUNCTION
void contact_patches_t::remove_pair(contact_pair_t &pair)
{
    pair.active = false;

    // modify the contact_pairs_access array
    // keep iterating in the row until the pair is found and shift all the elements to the left
    // then decrement the stride
    size_t &patch_stride = contact_pairs_access.stride(pair.patch.lid);
    bool found_node = false;
    for (size_t i = 0; i < patch_stride; i++)
    {
        const size_t &node_gid = contact_pairs_access(pair.patch.lid, i);
        if (node_gid == pair.node.gid)
        {
            found_node = true;
        } else if (found_node)
        {
            contact_pairs_access(pair.patch.lid, i - 1) = node_gid;
        }
    }
    patch_stride -= 1;
    assert(found_node && "Error: attempted to remove pair that doesn't exist in contact_pairs_access");
}  // end remove_pair

KOKKOS_FUNCTION
bool contact_patches_t::get_edge_pair(const ViewCArrayKokkos<double> &normal1, const ViewCArrayKokkos<double> &normal2,
                                      const size_t &node_gid, const double &del_t,
                                      ViewCArrayKokkos<double> &new_normal) const
{
    // Get the surface normal of the penetrating node by averaging all the normals at that node
    // we do this by looping through all the patches that the node is in
    double node_normal_arr[3];
    ViewCArrayKokkos<double> node_normal(&node_normal_arr[0], 3);
    // zero node normal
    for (int i = 0; i < 3; i++)
    {
        node_normal(i) = 0.0;
    }

    const size_t &num_patches = num_patches_in_node(node_gid);
    double local_normal_arr[3];
    ViewCArrayKokkos<double> local_normal(&local_normal_arr[0], 3);
    for (size_t i = 0; i < num_patches; i++)
    {
        const contact_patch_t &patch = contact_patches(patches_in_node(node_gid, i));
        // loop through the nodes in the patch until we find the node_gid the index matches with patch.xi and patch.eta
        for (int j = 0; j < contact_patch_t::num_nodes_in_patch; j++)
        {
            if (patch.nodes_gid(j) == node_gid)
            {
                // get the normal at that node
                patch.get_normal(patch.xi(j), patch.eta(j), del_t, local_normal);
                for (int k = 0; k < 3; k++)
                {
                    node_normal(k) += local_normal(k);
                }
                break;
            }
        }
    }
    // finish getting the average
    for (int i = 0; i < 3; i++)
    {
        node_normal(i) /= num_patches;
    }
    // Make it a unit vector
    double normal_norm = norm(node_normal);
    for (int i = 0; i < 3; i++)
    {
        node_normal(i) /= normal_norm;
    }

    // returning true means that a new pair should be formed
    // the pair that should be selected is the one that has the most negative dot product with the node normal
    // if normal1 is the most negative, then return false and make new_normal = normal1
    // if normal2 is the most negative, then return true and make new_normal = normal2
    // if the normals are the same, then return true and make new_normal the average between the two
    double dot1 = dot(normal1, node_normal);
    double dot2 = dot(normal2, node_normal);

    if (dot2 - tol <= dot1 && dot2 + tol >= dot1)
    {
        for (int i = 0; i < 3; i++)
        {
            new_normal(i) = (normal1(i) + normal2(i))/2.0;
        }

        // make new_normal a unit vector
        double new_normal_norm = norm(new_normal);
        for (int i = 0; i < 3; i++)
        {
            new_normal(i) /= new_normal_norm;
        }
        return true;
    } else if (dot1 < dot2)
    {
        for (int i = 0; i < 3; i++)
        {
            new_normal(i) = normal1(i);
        }
        return false;
    } else
    {
        for (int i = 0; i < 3; i++)
        {
            new_normal(i) = normal2(i);
        }
        return true;
    }
}  // end get_edge_pair

void contact_patches_t::force_resolution(const double &del_t)
{
    ViewCArrayKokkos<double> forces_view(&forces(0), num_active_pairs);
    for (int i = 0; i < max_iter; i++)
    {
        // find force increment for each pair
        // todo: having trouble breaking this into two parts, find force and apply force. It's not giving the right
        //       results so keep it serial for now.
        for (int j = 0; j < num_active_pairs; j++)
        {
            const size_t &node_gid = active_pairs(j);
            contact_pair_t &pair = contact_pairs(node_gid);

            // The reason why we are doing if statements inside the loop instead of loops inside of if statements is to
            // be able to have different contact types per pair. This makes it to where we can have a portion of the
            // mesh do frictionless contact and another portion do glue contact.
            if (pair.contact_type == contact_pair_t::contact_types::frictionless)
            {
                pair.frictionless_increment(*this, del_t);
                pair.distribute_frictionless_force(*this, 1.0);  // if not doing serial, then this would be called in the second loop
                forces_view(j) = pair.fc_inc;
            } // else if (pair.contact_type == contact_pair_t::contact_types::glue)
        }
        // Kokkos::fence();

        // check convergence (the force increments should be zero)
        if (norm(forces_view) <= tol)
        {
            break;
        }

        // distribute forces to nodes
        // todo: this loop is a little more complicated since we are accessing patches at the same time
        //       keep this serial until determining a way to use kokkos atomics
        // for (int j = 0; j < num_active_pairs; j++)
        // {
            // const size_t &node_gid = active_pairs(j);
            // contact_pair_t &pair = contact_pairs(node_gid);

            // if (pair.contact_type == contact_pair_t::contact_types::frictionless)
            // {
                // pair.distribute_frictionless_force(*this, 0.5);
            // } // else if (pair.contact_type == contact_pair_t::contact_types::glue)
        // }
    }
}  // end force_resolution

void contact_patches_t::remove_pairs(const double &del_t)
{
    for (int i = 0; i < num_active_pairs; i++)
    {
        const size_t &node_gid = active_pairs(i);
        contact_pair_t &pair = contact_pairs(node_gid);

        bool should_remove = false;
        if (pair.contact_type == contact_pair_t::contact_types::frictionless)
        {
            should_remove = pair.should_remove(del_t);
        } // else if (pair.contact_type == contact_pair_t::contact_types::glue)

        if (should_remove)
        {
            remove_pair(pair);
        }
    }
}
/// end of contact_patches_t member functions //////////////////////////////////////////////////////////////////////////

/// beginning of internal, not to be used anywhere else tests //////////////////////////////////////////////////////////
contact_patch_t::contact_patch_t() = default;

contact_patch_t::contact_patch_t(const ViewCArrayKokkos<double> &points, const ViewCArrayKokkos<double> &vel_points,
                                 const ViewCArrayKokkos<double> &acc_points)
{
    this->xi = CArrayKokkos<double>(4);
    this->eta = CArrayKokkos<double>(4);
    xi(0) = -1.0;
    xi(1) = 1.0;
    xi(2) = 1.0;
    xi(3) = -1.0;
    eta(0) = -1.0;
    eta(1) = -1.0;
    eta(2) = 1.0;
    eta(3) = 1.0;

    this->points = CArrayKokkos<double>(3, contact_patch_t::num_nodes_in_patch);
    this->vel_points = CArrayKokkos<double>(3, contact_patch_t::num_nodes_in_patch);
    this->acc_points = CArrayKokkos<double>(3, contact_patch_t::num_nodes_in_patch);
    this->mass_points = CArrayKokkos<double>(3, contact_patch_t::num_nodes_in_patch);
    this->internal_force = CArrayKokkos<double>(3, contact_patch_t::num_nodes_in_patch);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < num_nodes_in_patch; j++)
        {
            this->points(i, j) = points(i, j);
            this->vel_points(i, j) = vel_points(i, j);
            this->acc_points(i, j) = acc_points(i, j);
        }
    }
}

contact_node_t::contact_node_t() = default;

contact_node_t::contact_node_t(const ViewCArrayKokkos<double> &pos, const ViewCArrayKokkos<double> &vel,
                               const ViewCArrayKokkos<double> &acc, const double &mass)
{
    this->mass = mass;
    for (int i = 0; i < 3; i++)
    {
        this->pos(i) = pos(i);
        this->vel(i) = vel(i);
        this->acc(i) = acc(i);
    }
}

void run_contact_tests(contact_patches_t &contact_patches_obj, const mesh_t &mesh, const node_t &nodes,
                       const corner_t &corner, const simulation_parameters_t &sim_params)
{
    double err_tol = 1.0e-6;  // error tolerance

    // Testing get_contact_point with abnormal patch velocities. See contact_check_visual_through_reference.py.
    double test1_points_arr[3*4] = {1.0, 0.0, 0.0, 1.0,
                                    0.0, 0.0, 0.0, 0.0,
                                    0.0, 0.0, 1.0, 1.0};
    double test1_vels_arr[3*4] = {0.0, 0.0, 0.0, 0.0,
                                  1.0, 0.1, 0.2, 0.0,
                                  0.0, 0.0, 0.0, 0.0};
    double test1_acc_arr[3*4] = {0.0, 0.0, 0.0, 0.0,
                                 0.0, 0.0, 0.0, 0.0,
                                 0.0, 0.0, 0.0, 0.0};
    ViewCArrayKokkos<double> test1_points(&test1_points_arr[0], 3, 4);
    ViewCArrayKokkos<double> test1_vels(&test1_vels_arr[0], 3, 4);
    ViewCArrayKokkos<double> test1_accs(&test1_acc_arr[0], 3, 4);
    contact_patch_t test1_patch(test1_points, test1_vels, test1_accs);

    double test1_node_pos[3] = {0.25, 1.0, 0.2};
    double test1_node_vel[3] = {0.75, -1.0, 0.0};
    double test1_node_acc[3] = {0.0, 0.0, 0.0};
    ViewCArrayKokkos<double> test1_pos(&test1_node_pos[0], 3);
    ViewCArrayKokkos<double> test1_vel(&test1_node_vel[0], 3);
    ViewCArrayKokkos<double> test1_acc(&test1_node_acc[0], 3);
    contact_node_t test1_node(test1_pos, test1_vel, test1_acc, 1.0);

    double xi_val, eta_val, del_tc;
    bool is_hitting = test1_patch.get_contact_point(test1_node, xi_val, eta_val, del_tc);
    bool contact_check = test1_patch.contact_check(test1_node, 1.0, xi_val, eta_val, del_tc);
    std::cout << "\nTesting get_contact_point and contact_check:" << std::endl;
    std::cout << "-0.433241 -0.6 0.622161 vs. ";
    std::cout << xi_val << " " << eta_val << " " << del_tc << std::endl;
    assert(fabs(xi_val + 0.43324096) < err_tol);
    assert(fabs(eta_val + 0.6) < err_tol);
    assert(fabs(del_tc - 0.6221606424928471) < err_tol);
    assert(is_hitting);
    assert(contact_check);

    // Testing contact force calculation with the previous pair. See contact_check_visual_through_reference.py.
    std::cout << "\nTesting frictionless_increment:" << std::endl;
    double test1_internal_force_arr[3*4] = {0.0, 0.0, 0.0, 0.0,
                                            0.0, 0.0, 0.0, 0.0,
                                            0.0, 0.0, 0.0, 0.0};
    ViewCArrayKokkos<double> test1_internal_force(&test1_internal_force_arr[0], 3, 4);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            test1_patch.internal_force(i, j) = test1_internal_force(i, j);
            test1_patch.mass_points(i, j) = 1.0;
        }
    }

    double test1_node_internal_arr[3] = {0.0, 0.0, 0.0};
    ViewCArrayKokkos<double> test1_node_internal(&test1_node_internal_arr[0], 3);
    test1_node.mass = 1.0;
    for (int i = 0; i < 3; i++)
    {
        test1_node.internal_force(i) = test1_node_internal(i);
        test1_node.contact_force(i) = 0.0;
    }

    contact_patches_t test1_contact_patches;
    test1_contact_patches.contact_nodes = CArrayKokkos<contact_node_t> (4);
    test1_patch.nodes_gid = CArrayKokkos<size_t> (4);
    for (int i = 0; i < 4; i++)
    {
        contact_node_t &node = test1_contact_patches.contact_nodes(i);
        for (int j = 0; j < 3; j++)
        {
            node.contact_force(j) = 0.0;
        }

        test1_patch.nodes_gid(i) = i;
    }

    contact_pair_t test1_pair;
    test1_pair.patch = test1_patch;
    test1_pair.node = test1_node;
    test1_pair.xi = xi_val;
    test1_pair.eta = eta_val;
    test1_pair.del_tc = del_tc;

    double force_normal[3];
    ViewCArrayKokkos<double> force_n(&force_normal[0], 3);
    // Normal is not taken at del_tc, but is taken at the current time step; this is just for testing purposes
    test1_pair.patch.get_normal(test1_pair.xi, test1_pair.eta, test1_pair.del_tc, force_n);
    for (int i = 0; i < 3; i++)
    {
        test1_pair.normal(i) = force_n(i);
    }

    test1_pair.fc_inc = 0.5;
    test1_pair.frictionless_increment(test1_contact_patches, 1.0);
    std::cout << "-0.581465 -0.176368 0.858551 vs. ";
    std::cout << test1_pair.xi << " " << test1_pair.eta << " " << test1_pair.fc_inc << std::endl;
    assert(fabs(test1_pair.xi + 0.581465) < err_tol);
    assert(fabs(test1_pair.eta + 0.176368) < err_tol);
    assert(fabs(test1_pair.fc_inc - 0.858551) < err_tol);

    // Testing sort and get_contact_pairs
    std::string file_name = sim_params.mesh_input.file_path;
    std::string main_test = "contact_test.geo";
    std::string edge_case1 = "edge_case1.geo";
    std::string edge_case2 = "edge_case2.geo";
    std::string edge_case3 = "edge_case3.geo";

    std::cout << "\nTesting sort and get_contact_pairs:" << std::endl;
    if (file_name.find(main_test) != std::string::npos)
    {
        std::cout << "Patch with nodes 10 11 5 4 is paired with node 22" << std::endl;
        std::cout << "Patch with nodes 9 10 4 3 is paired with node 23" << std::endl;
        std::cout << "Patch with nodes 16 17 11 10 is paired with node 18" << std::endl;
        std::cout << "Patch with nodes 15 16 10 9 is paired with node 19" << std::endl;
        std::cout << "Patch with nodes 18 19 23 22 is paired with node 10" << std::endl;
        std::cout << "vs." << std::endl;
        contact_patches_obj.sort(mesh, nodes, corner);
        contact_patches_obj.get_contact_pairs(0.1);
        for (int i = 0; i < contact_patches_obj.num_contact_patches; i++)
        {
            for (int j = 0; j < contact_patches_obj.contact_pairs_access.stride(i); j++)
            {
                size_t node_gid = contact_patches_obj.contact_pairs_access(i, j);
                contact_pair_t &pair = contact_patches_obj.contact_pairs(node_gid);
                std::cout << "Patch with nodes ";
                for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
                {
                    std::cout << pair.patch.nodes_gid(k) << " ";
                }
                std::cout << "is paired with node " << pair.node.gid << std::endl;
            }
        }
        assert(contact_patches_obj.contact_pairs_access(2, 0) == 22);
        assert(contact_patches_obj.contact_pairs_access(6, 0) == 23);
        assert(contact_patches_obj.contact_pairs_access(10, 0) == 18);
        assert(contact_patches_obj.contact_pairs_access(14, 0) == 19);
        assert(contact_patches_obj.contact_pairs_access(18, 0) == 10);
    } else if (file_name.find(edge_case1) != std::string::npos)
    {
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 12" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 16" << std::endl;
        std::cout << "vs." << std::endl;
        contact_patches_obj.sort(mesh, nodes, corner);
        contact_patches_obj.get_contact_pairs(1.0);
        for (int i = 0; i < contact_patches_obj.num_contact_patches; i++)
        {
            for (int j = 0; j < contact_patches_obj.contact_pairs_access.stride(i); j++)
            {
                size_t node_gid = contact_patches_obj.contact_pairs_access(i, j);
                contact_pair_t &pair = contact_patches_obj.contact_pairs(node_gid);
                std::cout << "Patch with nodes ";
                for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
                {
                    std::cout << pair.patch.nodes_gid(k) << " ";
                }
                std::cout << "is paired with node " << pair.node.gid << std::endl;
            }
        }
        assert(contact_patches_obj.contact_pairs_access(6, 0) == 12);
        assert(contact_patches_obj.contact_pairs_access(6, 1) == 16);
    } else if (file_name.find(edge_case2) != std::string::npos)
    {
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 12" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 13" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 16" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 17" << std::endl;
        std::cout << "vs." << std::endl;
        contact_patches_obj.sort(mesh, nodes, corner);
        contact_patches_obj.get_contact_pairs(1.0);
        for (int i = 0; i < contact_patches_obj.num_contact_patches; i++)
        {
            for (int j = 0; j < contact_patches_obj.contact_pairs_access.stride(i); j++)
            {
                size_t node_gid = contact_patches_obj.contact_pairs_access(i, j);
                contact_pair_t &pair = contact_patches_obj.contact_pairs(node_gid);
                std::cout << "Patch with nodes ";
                for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
                {
                    std::cout << pair.patch.nodes_gid(k) << " ";
                }
                std::cout << "is paired with node " << pair.node.gid << std::endl;
            }
        }
        assert(contact_patches_obj.contact_pairs_access(6, 0) == 12);
        assert(contact_patches_obj.contact_pairs_access(6, 1) == 13);
        assert(contact_patches_obj.contact_pairs_access(6, 2) == 16);
        assert(contact_patches_obj.contact_pairs_access(6, 3) == 17);
    } else if (file_name.find(edge_case3) != std::string::npos)
    {
        std::cout << "Patch with nodes 6 7 1 0 is paired with node 12";
        std::cout << " ---> Pushback Direction: 0 -0.447214 0.894427" << std::endl;
        std::cout << "Patch with nodes 6 7 1 0 is paired with node 18";
        std::cout << " ---> Pushback Direction: 0 -0.447214 0.894427" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 13";
        std::cout << " ---> Pushback Direction: 0 0 1" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 14";
        std::cout << " ---> Pushback Direction: 0 0.447214 0.894427" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 19";
        std::cout << " ---> Pushback Direction: 0 0 1" << std::endl;
        std::cout << "Patch with nodes 7 8 2 1 is paired with node 20";
        std::cout << " ---> Pushback Direction: 0 0.447214 0.894427" << std::endl;
        std::cout << "vs." << std::endl;
        contact_patches_obj.sort(mesh, nodes, corner);
        contact_patches_obj.get_contact_pairs(1.0);
        for (int i = 0; i < contact_patches_obj.num_contact_patches; i++)
        {
            for (int j = 0; j < contact_patches_obj.contact_pairs_access.stride(i); j++)
            {
                size_t node_gid = contact_patches_obj.contact_pairs_access(i, j);
                contact_pair_t &pair = contact_patches_obj.contact_pairs(node_gid);
                std::cout << "Patch with nodes ";
                for (int k = 0; k < contact_patch_t::num_nodes_in_patch; k++)
                {
                    std::cout << pair.patch.nodes_gid(k) << " ";
                }
                std::cout << "is paired with node " << pair.node.gid;
                std::cout << " ---> Pushback Direction: ";
                for (int k = 0; k < 3; k++)
                {
                    std::cout << pair.normal(k) << " ";
                }
                std::cout << std::endl;
            }
        }
        assert(contact_patches_obj.contact_pairs_access(1, 0) == 12);
        assert(contact_patches_obj.contact_pairs_access(1, 1) == 18);
        assert(contact_patches_obj.contact_pairs_access(6, 0) == 13);
        assert(contact_patches_obj.contact_pairs_access(6, 1) == 14);
        assert(contact_patches_obj.contact_pairs_access(6, 2) == 19);
        assert(contact_patches_obj.contact_pairs_access(6, 3) == 20);
    }

    // Testing force resolution
    std::cout << "\nTesting force_resolution and remove_pairs:" << std::endl;
    if (file_name.find(main_test) != std::string::npos)
    {
        std::cout << "Penetrating node 22 has a fc_inc_total value of 0.148976" << std::endl;
        std::cout << "Penetrating node 23 has a fc_inc_total value of 0.148976" << std::endl;
        std::cout << "Penetrating node 18 has a fc_inc_total value of 0.148976" << std::endl;
        std::cout << "Penetrating node 19 has a fc_inc_total value of 0.148976" << std::endl;
        std::cout << "Penetrating node 10 has a fc_inc_total value of 0" << std::endl;
        std::cout << "vs." << std::endl;
        contact_patches_obj.force_resolution(0.1);

        double pen_node_sum = 0.0;
        double patch_node_sum = 0.0;
        bool seen_patch_node[26];
        for (int i = 0; i < contact_patches_obj.num_active_pairs; i++)
        {
            const size_t &node_gid = contact_patches_obj.active_pairs(i);
            const contact_pair_t &pair = contact_patches_obj.contact_pairs(node_gid);
            std::cout << "Penetrating node " << node_gid << " has a fc_inc_total_value of ";
            std::cout << pair.fc_inc_total << std::endl;

            pen_node_sum += pair.fc_inc_total;
            for (int j = 0; j < contact_patch_t::num_nodes_in_patch; j++)
            {
                const size_t &patch_node_gid = pair.patch.nodes_gid(j);
                if (!seen_patch_node[patch_node_gid] && pair.fc_inc_total > 0.0)
                {
                    seen_patch_node[patch_node_gid] = true;

                    const contact_node_t &patch_node = contact_patches_obj.contact_nodes(patch_node_gid);
                    patch_node_sum += patch_node.contact_force(2);
                }
            }
        }
        std::cout << "Penetrating node sum: " << pen_node_sum << std::endl;
        std::cout << "Patch node sum: " << patch_node_sum << std::endl;

        assert(fabs(pen_node_sum + patch_node_sum) < err_tol);
        assert(fabs(contact_patches_obj.contact_pairs(22).fc_inc_total - 0.148976) < err_tol);
        assert(fabs(contact_patches_obj.contact_pairs(23).fc_inc_total - 0.148976) < err_tol);
        assert(fabs(contact_patches_obj.contact_pairs(18).fc_inc_total - 0.148976) < err_tol);
        assert(fabs(contact_patches_obj.contact_pairs(19).fc_inc_total - 0.148976) < err_tol);
        assert(contact_patches_obj.contact_pairs(10).fc_inc_total < err_tol);

        // Testing remove_pairs
        contact_patches_obj.remove_pairs(0.1);
        assert(!contact_patches_obj.contact_pairs(10).active);
    }

    exit(0);
}
/// end of internal, not to be used anywhere else tests ////////////////////////////////////////////////////////////////
