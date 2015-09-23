#'
#' plot explained variance fractions as a function of # of eigen values
#' @export
#'
plotExplainedFractions <- function(outPdf, eigenVals, permutatedEigenVals, maxK) {
    pdf(file = outPdf)
    e = 0.000001
    eigenVals[which(eigenVals <= e)] = 0
    L = nrow(eigenVals)
    T = rep(0, L)
    for(k in 1:L) {
        T[k] = eigenVals[k] / sum(eigenVals)
    }
    ks = 1:maxK
    plot(ks, T[ks], type="h", xlab="K", col="gray", lty=2,
     ylab="Fraction", bty = "n", xaxt='n', ylim=c(0, max(T)*1.1), xlim=c(0.8, maxK+0.2))

    B = ncol(permutatedEigenVals)
    T_0 = matrix(0, L, B)
    for(b in 1:B) {
        pev = permutatedEigenVals[, b]
        pev[which(eigenVals <= e)] = 0
        for(k in 1:L) {
            T_0[k, b] = pev[k] / sum(pev)
        }
    
        lines(ks, T_0[ks, b], type="o", lwd=1, lty=1, col="pink", pch=19)
    }

    lines(ks, T[ks], type="o", lwd=3, lty=1, col="deepskyblue", pch=19)
    axis(side=1, at=ks, labels=as.character(ks))
    abline(h=mean(T_0[1,]), col="pink", lwd=1,lty=3)
    dev.off()
}
