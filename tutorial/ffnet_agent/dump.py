import numpy as np
import torch
import sys

def write_tensor(fp, tensor):
    if len(tensor.shape) == 2:
        # matrix
        fp.write("2 %d %d" % tensor.shape)
        for i in range(tensor.shape[0]):
            for j in range(tensor.shape[1]):
                fp.write(" ")
                fp.write(str(tensor[i, j]))
        fp.write("\n")
    else:
        # vector
        fp.write("1 %d" % tensor.shape)
        for i in range(tensor.shape[0]):
            fp.write(" ")
            fp.write(str(tensor[i]))
        fp.write("\n")

if __name__ == '__main__':
    model = torch.jit.load(sys.argv[1])
    model.eval()

    layers = [[]]

    for module in model.modules():
        if module.original_name == 'BatchNorm1d':
            net_weight = module.weight/(module.running_var+module.eps)**0.5
            net_bias = -module.weight*module.running_mean/(module.running_var+module.eps)**0.5 + module.bias
            layers[-1].append((torch.diag(net_weight).data.numpy(), net_bias.data.numpy()))
        elif module.original_name == 'Linear':
            weight, bias = list(module.parameters())
            layers[-1].append((weight.data.numpy(), bias.data.numpy()))
        elif module.original_name == 'Sigmoid':
            layers.append([])
        else:
            print(module.original_name)

    out = open(sys.argv[2], 'w')
    out.write('%d\n' % (2*len(layers)))
    for sublayers in layers:
        if len(sublayers) == 0:
            continue
        acc_weight, acc_bias = sublayers[0]
        for weight, bias in sublayers[1:]:
            acc_weight = np.matmul(weight, acc_weight)
            acc_bias = np.matmul(weight, acc_bias) + bias
        write_tensor(out, acc_weight)
        write_tensor(out, acc_bias)
    out.close()

    if len(sys.argv) >= 4:
        model.eval()
        test = open(sys.argv[3], 'w')
        
        fixed_in = [torch.tensor([[0.]*11]),
                    torch.tensor([[1e3] + [0.]*10]),
        ]
        for idx in range(10):
            if idx < len(fixed_in):
                test_in = fixed_in[idx]
            else:
                test_in = torch.rand(1,11)
            write_tensor(test, test_in[0,:].data.numpy())
            write_tensor(test, model(test_in)[0,:].detach().data.numpy())
        
        test.close()
