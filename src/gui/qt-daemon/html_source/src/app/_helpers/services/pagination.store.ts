import {Injectable} from '@angular/core';
import {Observable, BehaviorSubject} from 'rxjs';
import * as _ from 'lodash';
export interface Pages {
  page: number;
  offset: number;
  walletID: number;
}
@Injectable({
  providedIn: 'root'
})
export class PaginationStore {
  constructor(
  ) {}
  private subject = new BehaviorSubject<Pages[]>(null);
  pages$: Observable<Pages[]> = this.subject.asObservable();

  isForward(pages, currentPage) {
    const max = _.maxBy(pages, 'page');
    return !max || max.page < currentPage || max.page === currentPage;
  }
  setPage(pageNumber: number, offset: number, walletID: number) {
    let newPages: Pages[] = [];
    const pages = this.subject.getValue();
    if (pages && pages.length) {
      newPages = pages.slice(0);
    }
    newPages.push({page: pageNumber, offset, walletID});
    this.subject.next(newPages);
  }

  get value() {
    return this.subject.value;
  }

}
