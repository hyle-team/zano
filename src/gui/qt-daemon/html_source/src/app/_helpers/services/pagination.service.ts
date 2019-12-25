import { Injectable, NgZone } from '@angular/core';
import { VariablesService } from './variables.service';

@Injectable({
  providedIn: 'root'
})
export class PaginationService {

  constructor(
    private variables: VariablesService,
    private ngZone: NgZone
  ) { }

  paginate(currentPage = 1) {

    if (currentPage < 1) {
      currentPage = 1;
    } else if (currentPage > this.variables.currentWallet.totalPages) {
      currentPage = this.variables.currentWallet.totalPages;
    }

    let startPage: number, endPage: number;
    if (this.variables.currentWallet.totalPages <= this.variables.maxPages) {
      startPage = 1;
      endPage = this.variables.currentWallet.totalPages;
    } else {
      const maxPagesBeforeCurrentPage = Math.floor(this.variables.maxPages / 2);
      const maxPagesAfterCurrentPage = Math.ceil(this.variables.maxPages / 2) - 1;
      if (currentPage <= maxPagesBeforeCurrentPage) {
        startPage = 1;
        this.variables.currentWallet.totalPages > this.variables.maxPages
         ? endPage = this.variables.maxPages
          : endPage = this.variables.currentWallet.totalPages
        ;
      } else if (currentPage + maxPagesAfterCurrentPage >= this.variables.currentWallet.totalPages) {
        startPage = this.variables.currentWallet.totalPages - this.variables.maxPages + 1;
        endPage = this.variables.currentWallet.totalPages;
      } else {
        startPage = currentPage - maxPagesBeforeCurrentPage;
        endPage = currentPage + maxPagesAfterCurrentPage;
      }
    }
    this.ngZone.run(() => {
      this.variables.currentWallet.pages = Array.from(Array((endPage + 1) - startPage).keys()).map(i => startPage + i);
    });
  }
}
